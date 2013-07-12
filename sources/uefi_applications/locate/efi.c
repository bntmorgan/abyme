#include <efi.h>
#include <efilib.h>

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x0964e5b22,0x6459,0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b} }

EFI_SYSTEM_TABLE *systab;

EFI_STATUS read_file(IN CHAR16 *file_name) {
  EFI_STATUS status;
  UINTN index;
  UINTN handle_count;
  //UINTN scratch_buffer_size = 0;
  EFI_HANDLE *handle_buffer;
  EFI_FILE *root_fs;
  EFI_FILE *file_handle = 0;
  //EFI_FILE_INFO *file_info = 0;
  EFI_FILE_IO_INTERFACE *simple_file_system;
  EFI_GUID simple_file_system_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

  status = LibLocateHandle(ByProtocol, &simple_file_system_guid, NULL, &handle_count, &handle_buffer);
  if (EFI_ERROR(status)) {
    Print(L"FAILED LOL LocateHandle\n");
    return -1;
  }

  for (index = 0; index < handle_count; index++) {
    status = uefi_call_wrapper(systab->BootServices->HandleProtocol, 3, handle_buffer[index],
        &simple_file_system_guid, &simple_file_system);
    if (EFI_ERROR(status)) {
      Print(L"FAILED LOL HandleProtocol\n");
      continue;
    }
    
    status = uefi_call_wrapper(simple_file_system->OpenVolume,
        2, simple_file_system, (void **)&root_fs);
    if (EFI_ERROR(status)) {
      Print(L"FAILED LOL OpenVolume\n");
      continue;
    }
    
    status = uefi_call_wrapper(root_fs->Open, 5, root_fs, (void **)&file_handle, 
        L"\\caca.txt", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
      Print(L"FAILED LOL Open File\n");
      continue;
    }

  }

  FreePool(handle_buffer);

  return EFI_SUCCESS;
}


EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st)
{
  //EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  //EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

  InitializeLib(image_handle, st);
  systab = st;
  read_file(L"\\caca.txt");

  /*status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
    3,
    &gop_guid,
    NULL,
    &gop);

    Print(L"Framebuffer base is at %lx %d\n", gop->Mode->FrameBufferBase, status);

    EFI_GUID smm_guid = EFI_SMM_ACCESS_PROTOCOL_GUID;
    status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
    3,
    &smm_guid,
    NULL,
    &gop);

    Print(L"SMM %d\n", status);

    EFI_GUID smm_base_guid = EFI_SMM_BASE_PROTOCOL_GUID;
    status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
    3,
    &smm_base_guid,
    NULL,
    &gop);

    Print(L"SMM %d\n", status);*/
        
  /*EFI_GUID pci_io_guid = EFI_PCI_IO_PROTOCOL_GUID;
  EFI_PCI_IO_PROTOCOL *PciIo;
  status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
      3,
      &pci_io_guid,
      NULL,
      (void **)&PciIo);

  Print(L"PCI IO %d\n", status);
  if (status == 0) {
    uint8_t Character = 'c';
    uint32_t Row = 0, Column = 0;
    uint32_t VideoAddress = 0xB8000 + (Row * 80 + Column) * 2;
    uint16_t VideoCharacter = 0x0700 | Character;
    status = uefi_call_wrapper(PciIo->Mem.Write, 6, PciIo, EfiPciIoWidthUint16, 
        EFI_PCI_IO_PASS_THROUGH_BAR, VideoAddress, 1, &VideoCharacter);
    Row++;
    Column++;
    Print(L"PCI IO %d\n", status);
    status = uefi_call_wrapper(PciIo->Mem.Write, 6, PciIo, EfiPciIoWidthUint16, 
        EFI_PCI_IO_PASS_THROUGH_BAR, VideoAddress, 1, &VideoCharacter);
    Row++;
    Column++;
    status = uefi_call_wrapper(PciIo->Mem.Write, 6, PciIo, EfiPciIoWidthUint16, 
        EFI_PCI_IO_PASS_THROUGH_BAR, VideoAddress, 1, &VideoCharacter);
    Row++;
    Column++;
    status = uefi_call_wrapper(PciIo->Mem.Write, 6, PciIo, EfiPciIoWidthUint16, 
        EFI_PCI_IO_PASS_THROUGH_BAR, VideoAddress, 1, &VideoCharacter);

    uint32_t Value = 0x03;
    status = uefi_call_wrapper(PciIo->Io.Write, 6, PciIo, EfiPciIoWidthUint8,
        EFI_PCI_IO_PASS_THROUGH_BAR, 0x80, 1, &Value);
    Print(L"PCI IO %d\n", status);

    *((uint8_t *) (0xb8000 + 2)) = VideoCharacter;

  } else {
    Print(L"Error while retreiving the asshole\n");
  }*/

  return EFI_SUCCESS;
}

