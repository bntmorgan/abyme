#include <efi.h>
#include <efilib.h>

#include "pci.h"
#include "efi/efi_eric.h"
#include "debug.h"
#include "stdio.h"
#include "shell.h"

#include "efiw.h"

#define ERIC_VENDOR_ID  0x1AA5
#define ERIC_DEVICE_ID  0xE51C

void put_nothing(uint8_t c) {}

void eric_disable_debug(void){
  putc = &put_nothing;
}

//
// Protocol structure definition
//
protocol_eric proto = {
  .pci_addr = {
    0,
    0,
    0
  },
  0,
  0,
  0,
  0
};

EFI_STATUS rt_unload (IN EFI_HANDLE image);
EFI_GUID guid_eric = EFI_PROTOCOL_ERIC_GUID;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

  EFI_STATUS status;
  EFI_LOADED_IMAGE *image;
  uint32_t id;
  uint8_t *bar0, *bar1, *bar2, *rom;
  pci_device_addr addr;
  pci_device_info info;
  pci_device_info_ext info_ext;

  INFO("Experimental Intel 82579LM Ethernet driver initialization\n");
  if (pci_get_device(ERIC_VENDOR_ID, ERIC_DEVICE_ID, &addr)) {
    INFO("FAILED to locate ERIC\n");
    return EFI_NOT_FOUND;
  }

  id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);

  // Powerup PIO & MMI_extO
  pci_writew(id, PCI_CONFIG_COMMAND, 0x7);
  // pci_writeb(id, PCI_CONFIG_INTERRUPT_LINE, 0x5);

  INFO("COMMAND 0x%04x\n", pci_readw(id, PCI_CONFIG_COMMAND));

  pci_get_device_info(&info, id);
  pci_get_device_info_ext(&info_ext, id);
  pci_print_device_info(&info);
  pci_print_device_info_ext(&info_ext);

  // Read bar0

  pci_bar bar;
  pci_get_bar(&bar, id, 0);
  if (bar.flags & PCI_BAR_IO) {
    // Only Memory Mapped I/O supported
    INFO("Only Memory Mapped I/O supported\n");
    return -1;
  }
  bar0 = (uint8_t *)bar.u.address;

  pci_get_bar(&bar, id, 1);
  if (bar.flags & PCI_BAR_IO) {
    // Only Memory Mapped I/O supported
    INFO("Only Memory Mapped I/O supported\n");
    return -1;
  }
  bar1 = (uint8_t *)bar.u.address;

  pci_get_bar(&bar, id, 2);
  if (bar.flags & PCI_BAR_IO) {
    // Only Memory Mapped I/O supported
    INFO("Only Memory Mapped I/O supported\n");
    return -1;
  }
  bar2 = (uint8_t *)bar.u.address;

  rom = (uint8_t *)(uintptr_t)pci_readd(id,
      PCI_CONFIG_EXPANSION_ROM_BASE_ADDRESS);

  pci_writed(id, PCI_CONFIG_EXPANSION_ROM_BASE_ADDRESS,
      ((uint32_t)(uintptr_t)rom) | 1);

  INFO("BAR0 address (0x%08x)\n", bar0);
  dump(bar0, 2, 0x10, 8, (uint32_t)(uintptr_t)bar0, 2, 1);
  INFO("ROM address (0x%08x)\n", rom);
  dump(rom, 2, 0x10, 8, (uint32_t)(uintptr_t)rom, 2, 1);

  // Add an unload handler
  status = uefi_call_wrapper(BS->HandleProtocol, 4, &image_handle,
      &LoadedImageProtocol, (void*)&image);
  ASSERT (!EFI_ERROR(status));
  image->Unload = rt_unload;

  INFO("Unload handler added %x\n", (uintptr_t)image_handle);

  // Share the pci address in the uefi protocol
  pci_device_addr *pci_addr = &addr;
  proto.pci_addr.bus = pci_addr->bus;
  proto.pci_addr.device = pci_addr->device;
  proto.pci_addr.function = pci_addr->function;
  proto.bar0 = (uint32_t *)bar0;
  proto.bar1 = (uint32_t *)bar1;
  proto.bar2 = (uint32_t *)bar2;
  proto.rom = (uint32_t *)rom;

  // Add a protocol so someone can locate us
  status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &image_handle,
      &guid_eric, (EFI_INTERFACE_TYPE)NULL, &proto);
  ASSERT (!EFI_ERROR(status));

  uint32_t *fct = efi_allocate_pages(1);
  INFO("FAKE contexte table (0x%016X)\n", fct);

  // eric_disable_debug();

  return status;
}

EFI_STATUS rt_unload (IN EFI_HANDLE image) {
  return (EFI_SUCCESS);
}
