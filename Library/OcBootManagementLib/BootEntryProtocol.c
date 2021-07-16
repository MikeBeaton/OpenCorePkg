/** @file
  Boot Entry Protocol.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "BootEntryProtocolInternal.h"
#include "BootManagementInternal.h"

#include <Protocol/OcBootEntry.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcDebugLogLib.h>

/**
  Locate boot entry protocol handles.

  @param[in,out]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
  @param[in,out]     EntryProtocolHandleCount   Count of boot entry protocol handles.
**/
VOID
LocateBootEntryProtocolHandles (
  IN OUT EFI_HANDLE        **EntryProtocolHandles,
  IN OUT UINTN             *EntryProtocolHandleCount
  )
{
  EFI_STATUS        Status;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gOcBootEntryProtocolGuid,
    NULL,
    EntryProtocolHandleCount,
    EntryProtocolHandles
    );

  if (EFI_ERROR (Status)) {
    //
    // No loaded drivers is fine
    //
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "BEP: Error locating driver handles - %r\n", Status));
    }

    *EntryProtocolHandleCount = 0;
    *EntryProtocolHandles = NULL;
  }
}

/**
  Free boot entry protocol handles.

  @param[in,out]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
**/
VOID
FreeBootEntryProtocolHandles (
  EFI_HANDLE        **EntryProtocolHandles
  )
{
  if (*EntryProtocolHandles == NULL) {
    return;
  }

  FreePool (*EntryProtocolHandles);
  *EntryProtocolHandles = NULL;
}

/**
  Request bootable entries from installed boot entry protocol drivers.

  @param[in,out] BootContext                Context of filesystems.
  @param[in,out] FileSystem                 Filesystem to scan for entries.
  @param[in]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
  @param[in]     EntryProtocolHandleCount   Count of boot entry protocol handles.

  @retval EFI_STATUS for last created option.
**/
EFI_STATUS
AddEntriesFromBootEntryProtocol (
  IN OUT OC_BOOT_CONTEXT      *BootContext,
  IN OUT OC_BOOT_FILESYSTEM   *FileSystem,
  IN     EFI_HANDLE           *EntryProtocolHandles,
  IN     UINTN                EntryProtocolHandleCount
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    TempStatus;
  UINTN                         Index;
  OC_BOOT_ENTRY_PROTOCOL        *BootEntryProtocol;
  OC_BOOT_ENTRY                 *Entries;
  UINTN                         NumEntries;

  for (Index = 0; Index < EntryProtocolHandleCount; ++Index) {
    //
    // Previously marked as invalid.
    //
    if (EntryProtocolHandles[Index] == NULL) {
      continue;
    }

    TempStatus = gBS->HandleProtocol (
      EntryProtocolHandles[Index],
      &gOcBootEntryProtocolGuid,
      (VOID **) &BootEntryProtocol
      );

    if (EFI_ERROR (TempStatus)) {
      DEBUG ((DEBUG_ERROR, "BEP: HandleProtocol failed - %r\n", Status));
      continue;
    }

    if (BootEntryProtocol->Revision != OC_BOOT_ENTRY_PROTOCOL_REVISION) {
      DEBUG ((
        DEBUG_ERROR,
        "BEP: Invalid revision %u (!= %u) in loaded driver\n",
        BootEntryProtocol->Revision,
        OC_BOOT_ENTRY_PROTOCOL_REVISION
        ));
      EntryProtocolHandles[Index] = NULL;
      continue;
    }

    Status = BootEntryProtocol->GetBootEntries (
      FileSystem->Handle == OC_CUSTOM_FS_HANDLE ? NULL : FileSystem->Handle,
      &Entries,
      &NumEntries,
      NULL
      );

    if (EFI_ERROR (Status)) {
      //
      // No entries for any given driver on any given filesystem is normal
      //
      if (Status != EFI_NOT_FOUND) {
        DEBUG ((DEBUG_ERROR, "BEP: Unable to fetch boot entries - %r\n", Status));
      }
      continue;
    }

    //
    // TODO: Add entries to file system
    //

    FreePool (Entries);
  }

  return EFI_SUCCESS;
}
