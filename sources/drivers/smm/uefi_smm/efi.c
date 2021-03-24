/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

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

  Print(L"Vendor %s, Revision %d\n", ST->FirmwareVendor, ST->FirmwareRevision); 

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

EFI_STATUS load_image(CHAR16 *path, EFI_HANDLE parent_image_handle,
    EFI_HANDLE *loaded_image_handle, EFI_LOADED_IMAGE **ImageInfo) {
  UINTN NumberFileSystemHandles = 0;
  EFI_HANDLE *FileSystemHandles = NULL;
  UINTN Index;
  EFI_STATUS status;
  EFI_BLOCK_IO* BlkIo;
  EFI_DEVICE_PATH *FilePath;

  status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol,
      &BlockIoProtocol, NULL, &NumberFileSystemHandles, &FileSystemHandles);
  Print(L"NumberFileSystemHandles 0x%08x FileSystemHandles 0x%016X\n",
      NumberFileSystemHandles, FileSystemHandles);
  if (EFI_ERROR(status)) {
    Print(L"LocateHandleBuffer %r\n", status);
    return status;
  }

  for(Index = 0; Index < NumberFileSystemHandles; ++Index) {
    Print(L"Index %d\n", Index);
    status = uefi_call_wrapper(BS->HandleProtocol, 3, FileSystemHandles[Index],
        &BlockIoProtocol, (VOID**) &BlkIo);
    if (EFI_ERROR(status)) {
      Print(L"HandleProtocol %r\n", status);
      return status;
    }
    Print(L"BlkIo 0x%016X\n", BlkIo);
    if(!BlkIo->Media->RemovableMedia || BlkIo->Media->RemovableMedia) {
      FilePath = FileDevicePath(FileSystemHandles[Index],
          path);
      Print(L"after FileDevicePath - 0x%x - 0x%x\n", FilePath->Type,
          FilePath->SubType); 
      status = uefi_call_wrapper (BS->LoadImage, 6, FALSE, parent_image_handle,
          FilePath, NULL, 0, loaded_image_handle);
      Print(L"after LoadImage - 0x%08x - %d - %r \n", *loaded_image_handle,
          EFI_ERROR(status), status); 
      if(!EFI_ERROR(status)) {
        status = uefi_call_wrapper(BS->HandleProtocol, 3, *loaded_image_handle,
            &LoadedImageProtocol, (VOID **) ImageInfo);
        if (EFI_ERROR(status)) {
          Print(L"HandleProtocol %r\n", status);
          return status;
        }
        Print(L"Image base        : %lx\n", (*ImageInfo)->ImageBase);
        Print(L"Image file        : %s\n",
            DevicePathToStr((*ImageInfo)->FilePath));
        Print(L"Image size        : %lx\n", (*ImageInfo)->ImageSize);
        if(!EFI_ERROR(status)) {
          if((*ImageInfo)->ImageCodeType == EfiLoaderCode) {
            uefi_call_wrapper(BS->FreePool, 1, FilePath);
          }
          return EFI_SUCCESS;
        }
      }
    }
  }
  return EFI_NOT_FOUND;
}

EFI_STATUS smm_register(CHAR16 *path, EFI_HANDLE *image_handle_smm,
    EFI_SMM_BASE_PROTOCOL *smm_base) {
  UINTN NumberFileSystemHandles = 0;
  EFI_HANDLE *FileSystemHandles = NULL;
  UINTN Index;
  EFI_STATUS status;
  EFI_BLOCK_IO* BlkIo;
  EFI_DEVICE_PATH *FilePath;

  status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol,
      &BlockIoProtocol, NULL, &NumberFileSystemHandles, &FileSystemHandles);
  Print(L"NumberFileSystemHandles 0x%08x FileSystemHandles 0x%016X\n",
      NumberFileSystemHandles, FileSystemHandles);
  if (EFI_ERROR(status)) {
    Print(L"LocateHandleBuffer %r\n", status);
    return status;
  }

  for(Index = 0; Index < NumberFileSystemHandles; ++Index) {
    Print(L"Index %d\n", Index);
    status = uefi_call_wrapper(BS->HandleProtocol, 3, FileSystemHandles[Index],
        &BlockIoProtocol, (VOID**) &BlkIo);
    if (EFI_ERROR(status)) {
      Print(L"HandleProtocol %r\n", status);
      return status;
    }
    Print(L"BlkIo 0x%016X\n", BlkIo);
    if(!BlkIo->Media->RemovableMedia || BlkIo->Media->RemovableMedia) {
      FilePath = FileDevicePath(FileSystemHandles[Index],
          path);
      Print(L"after FileDevicePath - 0x%x - 0x%x\n", FilePath->Type,
          FilePath->SubType); 
      // Try to load with SMM BASE register() function YOLO
      status = uefi_call_wrapper(smm_base->Register, 6, smm_base, FilePath, 
          NULL, 0, &image_handle_smm, FALSE);
      Print(L"after register() - %r \n", status); 
      if(!EFI_ERROR(status)) {
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_NOT_FOUND;
}

EFI_STATUS
InitializeChild (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  EFI_STATUS status;
  EFI_GUID smm_base_guid = EFI_SMM_BASE_PROTOCOL_GUID;
  EFI_SMM_BASE_PROTOCOL *smm_base;
  uint8_t in_smm = 0;
  EFI_LOADED_IMAGE *loaded_image = NULL;
  EFI_HANDLE image_handle_smm;
  EFI_HANDLE loaded_image_handle;
  // Two != types of architecture
  CHAR16 *image_x86_64 = L"\\EFI\\uefi_drivers\\uefi_smm\\efi.efi";
  CHAR16 *image_i386 = L"\\EFI\\uefi_drivers\\uefi_smm_handler\\efi.efi";

  if (smm_is_smram_locked()) {
    Print(L"SMM is locked, we need to deal with UEFI SMM framedwork\n");
  }

  status = uefi_call_wrapper(systab->BootServices->LocateProtocol, 3,
      &smm_base_guid, NULL, (void **)&smm_base);

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

    Print(L"-== LOAD IMAGE TEST ON i386 and x86_64 images ==-\n");

    // Load the x86_64 image
    Print(L"Trying to load %s\n", image_x86_64);
    status = load_image(image_x86_64,
        image_handle, &loaded_image_handle, &loaded_image);
    if (EFI_ERROR(status)) {
      Print(L"load_image %r\n", status);
    } else {
      Print(L"Success ! Image first byte 0x%02x 0x4d is PE :)\n",
          *((uint8_t*)loaded_image->ImageBase));
    }
    // Load the i386 image
    Print(L"Trying to load %s\n", image_i386);
    status = load_image(image_i386,
        image_handle, &loaded_image_handle, &loaded_image);
    if (EFI_ERROR(status)) {
      Print(L"load_image %r\n", status);
    } else {
      Print(L"Success ! Image first byte 0x%02x 0x4d is PE :)\n",
          *((uint8_t*)loaded_image->ImageBase));
    }
    
    Print(L"-== SMBASE REGISTER TEST WITH THE PATH ON i386 and x86_64 images \
        ==-\n");
    Print(L"Trying to load %s\n", image_x86_64);
    status = smm_register(image_x86_64, &image_handle_smm, smm_base);
    if (EFI_ERROR(status)) {
      Print(L"load_image %r\n", status);
    } else {
      Print(L"Success ! Image first byte 0x%02x 0x4d is PE :)\n",
          *((uint8_t*)loaded_image->ImageBase));
    }
    // Load the i386 image
    Print(L"Trying to load %s\n", image_i386);
    status = smm_register(image_i386, &image_handle_smm, smm_base);
    if (EFI_ERROR(status)) {
      Print(L"load_image %r\n", status);
    } else {
      Print(L"Success ! Image first byte 0x%02x 0x4d is PE :)\n",
          *((uint8_t*)loaded_image->ImageBase));
    }
  }

  return EFI_SUCCESS;
}

EFI_DRIVER_ENTRY_POINT(InitializeChild);
