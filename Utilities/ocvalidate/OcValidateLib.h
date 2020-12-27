/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_USER_UTILITIES_OCVALIDATELIB_H
#define OC_USER_UTILITIES_OCVALIDATELIB_H

#include <sys/time.h>

#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Get current timestamp in milliseconds.
  
  @return     Current timestamp in milliseconds.
**/
INT64
GetCurrentTimestamp (
  VOID
  );

/**
  Check if a filesystem path contains only legal characters.

  @param[in]  Path                     Filesystem path to be checked.

  @retval     TRUE                     If Path only contains 0-9, A-Z, a-z, '_', '-', '.', '/', and '\'.
**/
BOOLEAN
AsciiFileSystemPathIsLegal (
  IN  CONST CHAR8  *Path
  );

/**
  Check if an OpenCore Configuration Comment contains only ASCII printable characters.

  @param[in]  Comment                  Comment to be checked.

  @retval     TRUE                     If Comment only contains ASCII printable characters.
**/
BOOLEAN
AsciiCommentIsLegal (
  IN  CONST CHAR8  *Comment
  );

/**
  Check if an OpenCore Configuration Identifier matches specific conventions.

  @param[in]  Identifier               Identifier to be checked.
  @param[in]  IsKernelPatchIdentifier  TRUE to perform special checks for Kernel->Patch->Identifier.

  @retval     TRUE                     If Identifier matches conventions.
**/
BOOLEAN
AsciiIdentifierIsLegal (
  IN  CONST CHAR8    *Identifier,
  IN  BOOLEAN        IsKernelIdentifier
  );

/**
  Check if an OpenCore Configuration Arch matches specific conventions.

  @param[in]  Arch                     Arch to be checked.
  @param[in]  IsKernelArch             Whether to perform special checks for Kernel->Scheme->KernelArch.

  @retval     TRUE                     If Arch matches conventions.
**/
BOOLEAN
AsciiArchIsLegal (
  IN  CONST CHAR8    *Arch,
  IN  BOOLEAN        IsKernelArch
  );

/**
  Check if an OpenCore Configuration Property contains only ASCII printable characters. Mainly used in device properties and NVRAM properties.

  @param[in]  Property                 Property to be checked.

  @retval     TRUE                     If Property only contains ASCII printable characters.
**/
BOOLEAN
AsciiPropertyIsLegal (
  IN  CONST CHAR8  *Property
  );

/**
  Check if a UEFI Driver matches specific conventions.

  @param[in]  Driver                   Driver to be checked.

  @retval     TRUE                     If path of Driver contains .efi suffix, and only contains 0-9, A-Z, a-z, '_', '-', '.', and '/'.
**/
BOOLEAN
AsciiUefiDriverIsLegal (
  IN  CONST CHAR8  *Driver
  );

/**
  Check if a device path in ASCII is valid.

  @param[in]  AsciiDevicePath          Device path to be checked.

  @retval     TRUE                     If AsciiDevicePath is valid.
**/
BOOLEAN
AsciiDevicePathIsLegal (
  IN  CONST CHAR8  *AsciiDevicePath
  );

/**
  Check if a GUID in ASCII is valid.

  @param[in]  AsciiGuid                GUID to be checked.

  @retval     TRUE                     If AsciiGuid has valid GUID format.
**/
BOOLEAN
AsciiGuidIsLegal (
  IN  CONST CHAR8  *AsciiGuid
  );

/**
  Check if a set of data has proper masking set.

  This function assumes identical sizes of Data and Mask, which must be ensured before calling.

  @param[in]  Data                     Data to be checked.
  @param[in]  Mask                     Mask to be paired with Data.
  @param[in]  Size                     Size of Data and Mask.

  @retval     TRUE                     If corresponding bits of Mask to Data are active (set to non-zero).
**/
BOOLEAN
DataHasProperMasking (
  IN  CONST VOID   *Data,
  IN  CONST VOID   *Mask,
  IN  UINTN        Size
  );

/**
  Check if an OpenCore binary patch is valid.

  If size of Find pattern cannot be zero, and size of Find pattern is different from that of Replace pattern, it is an error.
  If Mask/ReplaceMask is used, but its size is different from that of Find/Replace, it is an error.
  If Mask/ReplaceMask is used without corresponding bits being active for Find/Replace pattern, it is an error.

  @param[in]   PatchSection            Patch section to which the patch to be checked belongs.
  @param[in]   PatchIndex              Index of the patch to be checked.
  @param[in]   FindSizeCanBeZero       Whether size of Find pattern can be zero. Set to TRUE only when Kernel->Patch->Base is used and Find is empty.
  @param[in]   Find                    Find pattern to be checked.
  @param[in]   FindSize                Size of Find pattern to be checked.
  @param[in]   Replace                 Replace pattern to be checked.
  @param[in]   ReplaceSize             Size of Replace pattern to be checked.
  @param[in]   Mask                    Mask pattern to be checked.
  @param[in]   MaskSize                Size of Mask pattern to be checked.
  @param[in]   ReplaceMask             ReplaceMask pattern to be checked.
  @param[in]   ReplaceMaskSize         Size of ReplaceMask pattern to be checked.

  @return      Number of errors detected.
**/
UINT32
ValidatePatch (
  IN   CONST   CHAR8   *PatchSection,
  IN   UINT32          PatchIndex,
  IN   BOOLEAN         FindSizeCanBeZero,
  IN   CONST   UINT8   *Find,
  IN   UINT32          FindSize,
  IN   CONST   UINT8   *Replace,
  IN   UINT32          ReplaceSize,
  IN   CONST   UINT8   *Mask,
  IN   UINT32          MaskSize,
  IN   CONST   UINT8   *ReplaceMask,
  IN   UINT32          ReplaceMaskSize
  );

/**
  Report status of errors in the end of each checker functions.

  @param[in]  FuncName                 Checker function name. (__func__)
  @param[in]  ErrorCount               Number of errors to be returned.

  @return     Number of errors detected in one checker.
**/
UINT32
ReportError (
  IN  CONST CHAR8  *FuncName,
  IN  UINT32       ErrorCount
  );

#endif // OC_USER_UTILITIES_OCVALIDATELIB_H
