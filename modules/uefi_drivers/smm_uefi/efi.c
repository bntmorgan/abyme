#include <efi.h>
#include <efilib.h>
#include "SmmBase.h"
#include "SmmAccess.h"

#define SMRAM_MAP_SIZE 0x10

EFI_STATUS smm_is_smram_locked() {
  EFI_GUID smm_access_guid = EFI_SMM_ACCESS_PROTOCOL_GUID;
  EFI_SMM_ACCESS_PROTOCOL *smm_access;
  EFI_STATUS status;
  EFI_SMRAM_DESCRIPTOR SmramMap[SMRAM_MAP_SIZE];
  uint32_t SmramMapSize = SMRAM_MAP_SIZE * sizeof(EFI_SMRAM_DESCRIPTOR);
  uint32_t i;

  status = uefi_call_wrapper(BS->LocateProtocol, 3, &smm_access_guid, NULL,
                             (void **)&smm_access);
  if (EFI_ERROR(status)) {
    Print(L"EFI_SMM_ACCESS_PROTOCOL_GUID %r\n", status);
    return status;
  }
  // Displays all the SMRAM sections
  status = uefi_call_wrapper(smm_access->GetCapabilities, 3, smm_access,
      &SmramMapSize,
      SmramMap);
  Print(L"SmramMapSize 0x%08x 0x%02x\n", SmramMapSize, SmramMapSize /
      sizeof(EFI_SMRAM_DESCRIPTOR));
  if (EFI_ERROR(status)) {
    Print(L"LOL FAILED %r\n", status);
    return status;
  }
  for (i = 0; i < SmramMapSize / sizeof(EFI_SMRAM_DESCRIPTOR); i++) {
    Print(L"PhysicalStart : 0x%016X\nCpuStart : 0x%016X\nPhysicalSize :\
        0x%016X\nRegionState : 0x%016X\n", SmramMap[i].PhysicalStart,
        SmramMap[i].CpuStart, SmramMap[i].PhysicalSize,
        SmramMap[i].RegionState);
  }
  // Try to open the SMRAM yolo
  status = uefi_call_wrapper(smm_access->Open, 2, smm_access,
      SmramMapSize - 1);
  Print(L"LockState %d OpenState %d\n", smm_access->LockState,
      smm_access->OpenState);
  if (smm_access->LockState) {
    return 1;
  }
  return 0;
}

EFI_STATUS
InitializeChild (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  EFI_STATUS status;
  EFI_GUID smm_base_guid = EFI_SMM_BASE_PROTOCOL_GUID;
  EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
  EFI_SMM_BASE_PROTOCOL *smm_base;
  uint8_t in_smm = 0;
  EFI_DEVICE_PATH *dev_path = NULL;
  // EFI_DEVICE_PATH *dev_path_smm = NULL;
  EFI_LOADED_IMAGE *loaded_image = NULL;
  EFI_HANDLE image_handle_smm = NULL;
  uint32_t image_size = 0;
  // uint32_t image_size_smm = 0;

  if (smm_is_smram_locked()) {
    Print(L"SMM is locked, we need to deal with UEFI SMM framedwork\n");
  }

  status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
                             3,
                             &smm_base_guid,
                             NULL,
                             (void **)&smm_base);
  if (EFI_ERROR(status)) {
    Print(L"EFI_SMM_BASE_PROTOCOL 0x%08x\n", status);
    return status;
  }

  // Whe need o check if we are in SMM
  status = uefi_call_wrapper(smm_base->InSmm, 2, smm_base, &in_smm);
  if (EFI_ERROR(status)) {
    Print(L"InSmm() : %r\n", status);
    return status;
  }
  if (in_smm) {
    // Second pass, we are in SMM
  } else {
    // First pass we are in not in FUCKA SMM
    Print(L"YOLO DUDE this is the first pass 0x%08x\n", in_smm);
    // Get the device path
    status = uefi_call_wrapper(systab->BootServices->HandleProtocol, 3,
        image_handle, 
        &loaded_image_protocol, 
        (void **) &loaded_image);
    if (EFI_ERROR(status)) {
      Print(L"HandleProtocol(): %r\n", status);
      return status;
    }
    dev_path = loaded_image->FilePath;
    image_size = loaded_image->ImageSize;
    Print(L"Image base        : %lx\n", loaded_image->ImageBase);
    Print(L"Image file        : %s\n", DevicePathToStr(dev_path));
    Print(L"Image size        : %lx\n", image_size);
    Print(L"Image first byte 0x%02x 0x4d is PE :)\n",
        *((uint8_t*)loaded_image->ImageBase));

    // It is time to register the image YOLO
    status = uefi_call_wrapper(smm_base->Register,
        6,
        smm_base,
        NULL, 
        (VOID *) loaded_image->ImageBase, 
        image_size,
        &image_handle_smm,
        FALSE);
    if (EFI_ERROR(status)) {
      Print(L"SMM BASE register error: %r\n", status);
    }
  }

  return EFI_SUCCESS;
}

EFI_DRIVER_ENTRY_POINT(InitializeChild);
