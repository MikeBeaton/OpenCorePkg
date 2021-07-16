/** @file
  Boot Loader Specification-based Linux boot driver.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>

#include <Protocol/OcBootEntry.h>

STATIC
EFI_STATUS
InternalScanLoaderEntries (
  IN  EFI_HANDLE                      Device
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      TempStatus;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;

  EFI_FILE_PROTOCOL               *Directory;
  EFI_FILE_INFO                   *FileInfo;
  UINTN                           FileInfoSize;

  Root = NULL;

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &FileSystem
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "LNX: Missing filesystem - %r\n", Status));
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_BULK_INFO, "LNX: Invalid root volume - %r\n", Status));
    return Status;
  }

  Status = SafeFileOpen (Root, &Directory, L"\\loader\\entries", EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Ensure this is a directory.
  //
  FileInfo = GetFileInfo (Directory, &gEfiFileInfoGuid, 0, NULL);
  if (FileInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (!(FileInfo->Attribute & EFI_FILE_DIRECTORY)) {
    FreePool (FileInfo);
    return EFI_INVALID_PARAMETER;
  }
  FreePool (FileInfo);

  //
  // Allocate per-file FILE_INFO structure.
  //
  FileInfo = AllocatePool (SIZE_1KB);
  if (FileInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_NOT_FOUND;

  Directory->SetPosition (Directory, 0);

  do {
    //
    // EFI_FILE_INFO structures larger than 1KB are
    // unrealistic as the filename is the only variable.
    //
    FileInfoSize = SIZE_1KB - sizeof (CHAR16);
    TempStatus = Directory->Read (Directory, &FileInfoSize, FileInfo);
    if (EFI_ERROR (TempStatus)) {
      Directory->SetPosition (Directory, 0);
      FreePool (FileInfo);
      return TempStatus;
    }

    if (FileInfoSize > 0) {
      //
      // Skip ".*" and "auto-*" files, case sensitive;
      // follows systemd-boot logic.
      //
      if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) ||
        FileInfo->FileName[0] == L'.' ||
        StrnCmp (FileInfo->FileName, L"auto-", L_STR_LEN (L"auto-")) == 0 ||
        !OcUnicodeEndsWith (FileInfo->FileName, L".conf", TRUE)) {
        continue;
      }

      DEBUG ((DEBUG_INFO, "LNX: Ready to scan %s ...\n", FileInfo->FileName));

      Status = EFI_SUCCESS;
    }
  } while (FileInfoSize > 0);

  Directory->SetPosition (Directory, 0);
  FreePool (FileInfo);

  Root->Close (Root);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
OcGetLinuxBootEntries (
  IN   EFI_HANDLE               Device,
  OUT  OC_BOOT_ENTRY            **Entries,
  OUT  UINTN                    *NumEntries,
  IN   CHAR16                   *PrescanName OPTIONAL
  )
{
  EFI_STATUS                  Status;
  UINT32                      FileSystemPolicy;
  CONST EFI_PARTITION_ENTRY   *PartitionEntry;

  //
  // No custom entries
  //
  if (Device == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Disallow Apple filesystems, mainly to avoid needlessly
  // scanning multiple APFS partitions.
  //
  FileSystemPolicy = OcGetFileSystemPolicyType (Device);

  if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_APFS) != 0) {
    DEBUG ((DEBUG_INFO, "LNX: %a - not scanning\n", "APFS"));
    return EFI_NOT_FOUND;
  }

  if ((FileSystemPolicy & OC_SCAN_ALLOW_FS_HFS) != 0) {
    DEBUG ((DEBUG_INFO, "LNX: %a - not scanning\n", "HFS"));
    return EFI_NOT_FOUND;
  }

  //
  // Log TypeGUID and PARTUUID of the drive we're in
  //
  DEBUG_CODE_BEGIN ();
  PartitionEntry = OcGetGptPartitionEntry (Device);
  DEBUG ((
    DEBUG_INFO,
    "LNX: TypeGUID: %g PARTUUID: %g\n",
    PartitionEntry->PartitionTypeGUID,
    PartitionEntry->UniquePartitionGUID
    ));
  DEBUG_CODE_END ();

  //
  // Scan for boot loader spec entries
  //
  Status = InternalScanLoaderEntries (Device);

  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "LNX: Nothing found\n"));
  }

  return EFI_NOT_FOUND;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
mLinuxBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  OcGetLinuxBootEntries
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
    &ImageHandle,
    &gOcBootEntryProtocolGuid,
    &mLinuxBootEntryProtocol,
    NULL
    );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
