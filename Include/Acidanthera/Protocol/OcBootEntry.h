/** @file
  OpenCore boot entry protocol.

  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_BOOT_ENTRY_PROTOCOL_H
#define OC_BOOT_ENTRY_PROTOCOL_H

#include <Uefi.h>
#include <Library/OcBootManagementLib.h>

#include <Protocol/SimpleFileSystem.h>

///
/// 8604716E-ADD4-45B4-8495-08E36D497F4F
///
#define OC_BOOT_ENTRY_PROTOCOL_GUID                                                 \
  {                                                                                 \
    0x8604716E, 0xADD4, 0x45B4, { 0x84, 0x95, 0x08, 0xE3, 0x6D, 0x49, 0x7F, 0x4F }  \
  }

///
/// OC_BOOT_ENTRY_PROTOCOL revision
///
#define OC_BOOT_ENTRY_PROTOCOL_REVISION 0

///
/// Forward declaration of OC_BOOT_ENTRY_PROTOCOL structure.
///
typedef struct OC_BOOT_ENTRY_PROTOCOL_ OC_BOOT_ENTRY_PROTOCOL;

/**
  Return list of OpenCore boot entries associated with filesystem.

  @param[in]  Filesystem        The filesystem to scan. NULL is passed in to request
                                custom entries. All implementations must support a NULL
                                input value, but should immediately return EFI_NOT_FOUND
                                if they do not provide any custom entries.
  @param[out] BootEntries       List of boot entries associated with the filesystem.
                                On EFI_SUCCESS BootEntries must be freed by the caller
                                with FreePool after use.
                                Each individual boot entry should eventually be freed
                                by FreeBootEntry (as for boot entries created within
                                OC itself).
                                This value does not point to allocated memory on return,
                                if any status other than EFI_SUCCESS was returned.
  @param[out] NumEntries        The number of entries returned in the BootEntries list.
                                If any status other than EFI_SUCCESS was returned, this
                                value may not have been initialised and should be ignored
                                by the caller.
  @param[in]  PrescanName       If present, only the first entry with this name
                                (counting from zero upwards, in the same order in which
                                the full entry list would have been returned), if any,
                                will be created and returned, in a list of length one.
                                This is an invalid parameter for the NULL Filesystem;
                                when Filesystem is NULL, implementations can and should
                                ignore this parameter.
                                For any non-NULL Filesystem parameters which could
                                return more than one entry without this parameter,
                                implementations must support this parameter.

  @retval EFI_SUCCESS           At least one matching entry was found, and the list and
                                count of boot entries has been returned.
  @retval EFI_NOT_FOUND         No matching boot entries were found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failure.
  @retval other                 An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *OC_GET_BOOT_ENTRIES) (
  IN   OC_BOOT_FILESYSTEM       *Filesystem,
  OUT  OC_BOOT_ENTRY            **Entries,
  OUT  UINTN                    *NumEntries,
  IN   CHAR16                   *PrescanName OPTIONAL
  );

///
/// The structure exposed by the OC_BOOT_ENTRY_PROTOCOL.
///
struct OC_BOOT_ENTRY_PROTOCOL_ {
  UINTN                 Revision;
  OC_GET_BOOT_ENTRIES   GetBootEntries;
};

extern EFI_GUID gOcBootEntryProtocolGuid;

#endif // OC_BOOT_ENTRY_PROTOCOL_H
