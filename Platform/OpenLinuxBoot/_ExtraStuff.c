//

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcLinuxBoot.h>

#define LINUX_KERNEL_VERSION_STRING_MAX_SIZE 128

//
// Extract kernel version and Linux type from bootable kernel
//
STATIC
GetKernelVersion (
  IN  EFI_FILE_PROTOCOL   *File,
  OUT CHAR8               **KernelVersion
  )
{
  UINT16      Offset;
  EFI_STATUS  Status;
  CHAR8       VersionString[128];

  *KernelVersion = AllocatePool (LINUX_KERNEL_VERSION_STRING_MAX_SIZE);
  if (KernelVersion == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetFileData(File, 0x20E, sizeof (Offset), &Offset)
  if (EFI_ERROR (Status)) {
    FreePool (KernelVersion);
    return Status;

  }
  Status = GetFileData(File, Offset + 0x200, sizeof (VersionString), &VersionString);
  if (EFI_ERROR (Status)) {
    FreePool (KernelVersion);
    return Status;
  }

  VersionString[ARRAY_SIZE (VersionString - 1)] = '\0';
  return EFI_SUCCESS;
}

STATIC
OpenFiles (
  VOID
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *VmlinuzFile,
  CHAR8               *KernelVersion;
  CHAR8               *Label;

  //
  // Open file:
  //  Check InternalFileExists (which needs to be made into SafeFileExists) and
  //  SafeFileOpen
  //

  // Search for /bin/sh
  Status = FindFile(L"/bin/sh");
  if (EFI_ERROR (Status)) {
    OC_DEBUG ((DEBUG_INFO, "LNX: %s not present", L"/bin/sh"));
    return Status;
  }

  // Open /boot/vmlinuz
  Status = OpenFile(L"/boot/vmlinuz");
  if (EFI_ERROR (Status)) {
    OC_DEBUG ((DEBUG_INFO, "LNX: %s not present", L"/boot/vmlinuz"));
    return Status;
  }

  Status = GetKernelVersion (VmlinuzFile, KernelVersion);
  CloseFile(VmlinuzFile);
  if (!Status) {
    return Status;
  }

  Status = FindFile(L"/boot/initrd.img");
  if (EFI_ERROR (Status)) {
    OC_DEBUG ((DEBUG_INFO, "LNX: %s not present", L"/boot/initrd.img"));

    FreePool (KernelVersion);
    return Status;
  }

  Label = CreateLinuxLabel (KernelVersion);
  FreePool (KernelVersion);
}
