#include <efi.h>
#include <efilib.h>
#include "efi/efiapi_1_1.h"
#include "efi/efi_1_1.h"

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
    { 0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1} }

#define EFI_SMM_BASE_PROTOCOL_GUID \
    { 0x1390954d, 0xda95, 0x4227, {0x93, 0x28, 0x72, 0x82, 0xc2, 0x17, 0xda, 0xa8} }

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
        EFI_STATUS status;
        EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
        EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

        InitializeLib(image_handle, systab);

        status = uefi_call_wrapper(GET_EFI_BOOT_SERVICES_1_1(systab->BootServices)->LocateProtocol,
                                   3,
                                   &gop_guid,
                                   NULL,
                                   &gop);

        Print(L"Framebuffer base is at %lx %d\n", gop->Mode->FrameBufferBase, status);
        
        EFI_GUID smm_guid = EFI_SMM_ACCESS_PROTOCOL_GUID;
        status = uefi_call_wrapper(GET_EFI_BOOT_SERVICES_1_1(systab->BootServices)->LocateProtocol,
                                   3,
                                   &smm_guid,
                                   NULL,
                                   &gop);

        Print(L"SMM %d\n", status);
        
        EFI_GUID smm_base_guid = EFI_SMM_BASE_PROTOCOL_GUID;
        status = uefi_call_wrapper(GET_EFI_BOOT_SERVICES_1_1(systab->BootServices)->LocateProtocol,
                                   3,
                                   &smm_base_guid,
                                   NULL,
                                   &gop);

        Print(L"SMM %d\n", status);

        return EFI_SUCCESS;
}

