/** @file
  Boot Loader Specification-based Linux boot driver.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcDebugLogLib.h>

#include <Protocol/OcBootEntry.h>

//
// Disallow Apple filesystems, mainly to avoid scanning multiple APFS partitions.
//
#define LINUX_SCAN_DISALLOW \
  (OC_SCAN_ALLOW_FS_APFS | OC_SCAN_ALLOW_FS_HFS)

STATIC
EFI_STATUS
EFIAPI
OcGetLinuxBootEntries (
  IN   OC_BOOT_FILESYSTEM       *Filesystem,
  OUT  OC_BOOT_ENTRY            **Entries,
  OUT  UINTN                    *NumEntries,
  IN   CHAR16                   *PrescanName OPTIONAL
  )
{
  UINT32                      FileSystemPolicy;
  CONST EFI_PARTITION_ENTRY   *PartitionEntry;

  //
  // No custom entries
  //
  if (Filesystem == NULL) {
    // // TODO: Remove debug msg
    // DEBUG ((DEBUG_INFO, "LNX: No custom entries!\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Disallow unwanted filesystems; we will only be called
  // with filesystems already allowed by OC scan policy.
  //
  FileSystemPolicy = OcGetFileSystemPolicyType (Filesystem->Handle);
  if ((LINUX_SCAN_DISALLOW & FileSystemPolicy) != 0) {
    DEBUG ((DEBUG_INFO, "LNX: Disallowed file system policy (%x/%x) for %p\n", FileSystemPolicy, LINUX_SCAN_DISALLOW, Filesystem->Handle));
    return EFI_NOT_FOUND;
  }

  //
  // Log TypeGUID and PARTUUID of the drive we're in
  //
  DEBUG_CODE_BEGIN ();
  PartitionEntry = OcGetGptPartitionEntry (Filesystem->Handle);
  DEBUG ((
    DEBUG_INFO,
    "LNX: TypeGUID: %g PARTUUID: %g\n",
    PartitionEntry->PartitionTypeGUID,
    PartitionEntry->UniquePartitionGUID
    ));
  DEBUG_CODE_END ();

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
