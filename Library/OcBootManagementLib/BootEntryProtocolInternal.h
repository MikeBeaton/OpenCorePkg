/** @file
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef BOOT_ENTRY_PROTOCOL_INTERNAL_H
#define BOOT_ENTRY_PROTOCOL_INTERNAL_H

#include <Uefi.h>
#include <Library/OcBootManagementLib.h>

/**
  Locate boot entry protocol handles.

  @param[in,out]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
  @param[in,out]     EntryProtocolHandleCount   Count of boot entry protocol handles.
**/
VOID
LocateBootEntryProtocolHandles (
  IN OUT EFI_HANDLE        **EntryProtocolHandles,
  IN OUT UINTN             *EntryProtocolHandleCount
  );

/**
  Free boot entry protocol handles.

  @param[in,out]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
**/
VOID
FreeBootEntryProtocolHandles (
  EFI_HANDLE        **EntryProtocolHandles
  );

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
  );

#endif // BOOT_ENTRY_PROTOCOL_INTERNAL_H
