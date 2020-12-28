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

#include "ocvalidate.h"
#include "OcValidateLib.h"

#include <Library/OcConsoleLib.h>

/**
  Callback funtion to verify whether one UEFI driver is duplicated in UEFI->Drivers.

  @param[in]  PrimaryDriver    Primary driver to be checked.
  @param[in]  SecondaryDriver  Secondary driver to be checked.

  @retval     TRUE             If PrimaryDriver and SecondaryDriver are duplicated.
**/
STATIC
BOOLEAN
UEFIDriverHasDuplication (
  IN  CONST VOID  *PrimaryDriver,
  IN  CONST VOID  *SecondaryDriver
  )
{
  CONST OC_STRING           *UEFIPrimaryDriver;
  CONST OC_STRING           *UEFISecondaryDriver;
  CONST CHAR8               *UEFIDriverPrimaryString;
  CONST CHAR8               *UEFIDriverSecondaryString;

  UEFIPrimaryDriver         = *(CONST OC_STRING **) PrimaryDriver;
  UEFISecondaryDriver       = *(CONST OC_STRING **) SecondaryDriver;
  UEFIDriverPrimaryString   = OC_BLOB_GET (UEFIPrimaryDriver);
  UEFIDriverSecondaryString = OC_BLOB_GET (UEFISecondaryDriver);

  return StringIsDuplicated ("UEFI->Drivers", UEFIDriverPrimaryString, UEFIDriverSecondaryString);
}

UINT32
CheckUEFI (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                    ErrorCount;
  UINT32                    Index;
  UINT32                    IndexOpenUsbKbDxeEfiDriver;
  UINT32                    IndexPs2KeyboardDxeEfiDriver;
  OC_UEFI_CONFIG            *UserUefi;
  OC_MISC_CONFIG            *UserMisc;
  CONST CHAR8               *Driver;
  CONST CHAR8               *TextRenderer;
  CONST CHAR8               *ConsoleMode;
  CONST CHAR8               *PointerSupportMode;
  CONST CHAR8               *KeySupportMode;
  BOOLEAN                   HasOpenRuntimeEfiDriver;
  BOOLEAN                   HasOpenUsbKbDxeEfiDriver;
  BOOLEAN                   HasPs2KeyboardDxeEfiDriver;
  BOOLEAN                   IsRequestBootVarRoutingEnabled;
  BOOLEAN                   IsKeySupportEnabled;
  BOOLEAN                   IsTextRendererSystem;
  BOOLEAN                   IsClearScreenOnModeSwitchEnabled;
  BOOLEAN                   IsIgnoreTextInGraphicsEnabled;
  BOOLEAN                   IsReplaceTabWithSpaceEnabled;
  BOOLEAN                   IsSanitiseClearScreenEnabled;
  BOOLEAN                   IsPointerSupportEnabled;
  CONST CHAR8               *Resolution;
  UINT32                    UserWidth;
  UINT32                    UserHeight;
  UINT32                    UserBpp;
  BOOLEAN                   UserSetMax;
  CONST CHAR8               *AsciiAudioDevicePath;

  DEBUG ((DEBUG_VERBOSE, "config loaded into UEFI checker!\n"));

  ErrorCount                       = 0;
  IndexOpenUsbKbDxeEfiDriver       = 0;
  IndexPs2KeyboardDxeEfiDriver     = 0;
  UserUefi                         = &Config->Uefi;
  UserMisc                         = &Config->Misc;
  HasOpenRuntimeEfiDriver          = FALSE;
  HasOpenUsbKbDxeEfiDriver         = FALSE;
  HasPs2KeyboardDxeEfiDriver       = FALSE;
  IsRequestBootVarRoutingEnabled   = UserUefi->Quirks.RequestBootVarRouting;
  IsKeySupportEnabled              = UserUefi->Input.KeySupport;
  IsPointerSupportEnabled          = UserUefi->Input.PointerSupport;
  PointerSupportMode               = OC_BLOB_GET (&UserUefi->Input.PointerSupportMode);
  KeySupportMode                   = OC_BLOB_GET (&UserUefi->Input.KeySupportMode);
  IsClearScreenOnModeSwitchEnabled = UserUefi->Output.ClearScreenOnModeSwitch;
  IsIgnoreTextInGraphicsEnabled    = UserUefi->Output.IgnoreTextInGraphics;
  IsReplaceTabWithSpaceEnabled     = UserUefi->Output.ReplaceTabWithSpace;
  IsSanitiseClearScreenEnabled     = UserUefi->Output.SanitiseClearScreen;
  TextRenderer                     = OC_BLOB_GET (&UserUefi->Output.TextRenderer);
  IsTextRendererSystem             = FALSE;
  ConsoleMode                      = OC_BLOB_GET (&UserUefi->Output.ConsoleMode);
  Resolution                       = OC_BLOB_GET (&UserUefi->Output.Resolution);
  AsciiAudioDevicePath             = OC_BLOB_GET (&UserUefi->Audio.AudioDevice);

  //
  // Sanitise strings.
  //
  if (AsciiStrCmp (TextRenderer, "BuiltinGraphics") != 0
    && AsciiStrCmp (TextRenderer, "BuiltinText") != 0
    && AsciiStrCmp (TextRenderer, "SystemGraphics") != 0
    && AsciiStrCmp (TextRenderer, "SystemText") != 0
    && AsciiStrCmp (TextRenderer, "SystemGeneric") != 0) {
    DEBUG ((DEBUG_WARN, "UEFI->Output->TextRenderer is illegal (Can only be BuiltinGraphics, BuiltinText, SystemGraphics, SystemText, or SystemGeneric)!\n"));
    ++ErrorCount;
  } else if (AsciiStrnCmp (TextRenderer, "System", L_STR_LEN ("System")) == 0) {
    //
    // Check whether TextRenderer has System prefix.
    //
    IsTextRendererSystem = TRUE;
  }

  //
  // If FS restrictions is enabled but APFS FS scanning is disabled, it is an error.
  //
  if (UserUefi->Apfs.EnableJumpstart
    && (UserMisc->Security.ScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) != 0
    && (UserMisc->Security.ScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0) {
    DEBUG ((DEBUG_WARN, "UEFI->APFS->EnableJumpstart is enabled, but Misc->Security->ScanPolicy does not allow APFS scanning!\n"));
    ++ErrorCount;
  }

  if (AsciiAudioDevicePath[0] != '\0' && !AsciiDevicePathIsLegal (AsciiAudioDevicePath)) {
    DEBUG ((DEBUG_WARN, "UEFI->Audio->AudioDevice is borked! Please check the information above!\n"));
    ++ErrorCount;
  }

  for (Index = 0; Index < UserUefi->Drivers.Count; ++Index) {
    Driver = OC_BLOB_GET (UserUefi->Drivers.Values[Index]);

    //
    // Sanitise strings.
    //
    if (!AsciiUefiDriverIsLegal (Driver)) {
      DEBUG ((DEBUG_WARN, "UEFI->Drivers[%u] contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    if (AsciiStrCmp (Driver, "OpenRuntime.efi") == 0) {
      HasOpenRuntimeEfiDriver = TRUE;
    }
    if (AsciiStrCmp (Driver, "OpenUsbKbDxe.efi") == 0) {
      HasOpenUsbKbDxeEfiDriver   = TRUE;
      IndexOpenUsbKbDxeEfiDriver = Index;
    }
    if (AsciiStrCmp (Driver, "Ps2KeyboardDxe.efi") == 0) {
      HasPs2KeyboardDxeEfiDriver   = TRUE;
      IndexPs2KeyboardDxeEfiDriver = Index;
    }
  }

  //
  // Check duplicated Drivers.
  //
  ErrorCount += FindArrayDuplication (
    UserUefi->Drivers.Values,
    UserUefi->Drivers.Count,
    sizeof (UserUefi->Drivers.Values[0]),
    UEFIDriverHasDuplication
    );

  if (IsPointerSupportEnabled && AsciiStrCmp (PointerSupportMode, "ASUS") != 0) {
    DEBUG ((DEBUG_WARN, "UEFI->Input->PointerSupport is enabled, but PointerSupportMode is not ASUS!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (KeySupportMode, "Auto") != 0
    && AsciiStrCmp (KeySupportMode, "V1") != 0
    && AsciiStrCmp (KeySupportMode, "V2") != 0
    && AsciiStrCmp (KeySupportMode, "AMI") != 0) {
    DEBUG ((DEBUG_WARN, "UEFI->Input->KeySupportMode is illegal (Can only be Auto, V1, V2, AMI)!\n"));
    ++ErrorCount;
  }

  if (IsRequestBootVarRoutingEnabled) {
    if (!HasOpenRuntimeEfiDriver) {
      DEBUG ((DEBUG_WARN, "UEFI->Quirks->RequestBootVarRouting is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
  }

  if (IsKeySupportEnabled) {
    if (HasOpenUsbKbDxeEfiDriver) {
      DEBUG ((DEBUG_WARN, "OpenUsbKbDxe.efi at UEFI->Drivers[%u] should NEVER be used together with UEFI->Input->KeySupport!\n", IndexOpenUsbKbDxeEfiDriver));
      ++ErrorCount;
    }
  } else {
    if (HasPs2KeyboardDxeEfiDriver) {
      DEBUG ((DEBUG_WARN, "UEFI->Input->KeySupport should be enabled when Ps2KeyboardDxe.efi is in use!\n"));
      ++ErrorCount;
    }
  }

  if (HasOpenUsbKbDxeEfiDriver && HasPs2KeyboardDxeEfiDriver) {
    DEBUG ((
      DEBUG_WARN,
      "OpenUsbKbDxe.efi at UEFI->Drivers[%u], and Ps2KeyboardDxe.efi at UEFI->Drivers[%u], should NEVER co-exist!\n",
      IndexOpenUsbKbDxeEfiDriver,
      IndexPs2KeyboardDxeEfiDriver
      ));
    ++ErrorCount;
  }

  if (!IsTextRendererSystem) {
    if (IsClearScreenOnModeSwitchEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ClearScreenOnModeSwitch is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
    if (IsIgnoreTextInGraphicsEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->IgnoreTextInGraphics is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
    if (IsReplaceTabWithSpaceEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ReplaceTabWithSpace is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
    if (IsSanitiseClearScreenEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->SanitiseClearScreen is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
  }

  //
  // Parse Output->ConsoleMode by calling OpenCore libraries.
  //
  OcParseConsoleMode (
    ConsoleMode,
    &UserWidth,
    &UserHeight,
    &UserSetMax
    );
  if (ConsoleMode[0] != '\0'
    && !UserSetMax
    && (UserWidth == 0 || UserHeight == 0)) {
    DEBUG ((DEBUG_WARN, "UEFI->Output->ConsoleMode is borked, please check Configurations.pdf!\n"));
    ++ErrorCount;
  }

  //
  // Parse Output->Resolution by calling OpenCore libraries.
  //
  OcParseScreenResolution (
    Resolution,
    &UserWidth,
    &UserHeight,
    &UserBpp,
    &UserSetMax
    );
  if (Resolution[0] != '\0'
    && !UserSetMax
    && (UserWidth == 0 || UserHeight == 0)) {
    DEBUG ((DEBUG_WARN, "UEFI->Output->Resolution is borked, please check Configurations.pdf!\n"));
    ++ErrorCount;
  }

  return ReportError (__func__, ErrorCount);
}
