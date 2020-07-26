#include <efi.h>
#include <efilib.h>

#include "pci.h"
#include "efi/efi_eric.h"
#include "debug.h"
#include "stdio.h"
#include "shell.h"

void put_nothing(uint8_t c) {}

void eric_disable_debug(void){
  putc = &put_nothing;
}

struct dmiuests {
  uint32_t r0:4;
  uint32_t dlpes:1;
  uint32_t r1:7;
  uint32_t ptlps:1;
  uint32_t r2:1;
  uint32_t cts:1;
  uint32_t r3:1;
  uint32_t ucs:1;
  uint32_t ros:1;
  uint32_t mtlps:1;
  uint32_t r4:1;
  uint32_t ures:1;
  uint32_t r5:11;
};

void print_dmiuests(struct dmiuests *dmiuests) {
  printk("r0(0x%x)\n", dmiuests->r0);
  printk("dlpes(0x%x)\n", dmiuests->dlpes);
  printk("r1(0x%x)\n", dmiuests->r1);
  printk("ptlps(0x%x)\n", dmiuests->ptlps);
  printk("r2(0x%x)\n", dmiuests->r2);
  printk("cts(0x%x)\n", dmiuests->cts);
  printk("r3(0x%x)\n", dmiuests->r3);
  printk("ucs(0x%x)\n", dmiuests->ucs);
  printk("ros(0x%x)\n", dmiuests->ros);
  printk("mtlps(0x%x)\n", dmiuests->mtlps);
  printk("r4(0x%x)\n", dmiuests->r4);
  printk("ures(0x%x)\n", dmiuests->ures);
  printk("r5(0x%x)\n", dmiuests->r5);
}

#define PCI_CONFIG_DMIBAR             0x68
#define PCI_CONFIG_DPR                0x5c
#define PCI_CONFIG_MESEG_BASE         0x70
#define PCI_CONFIG_MESEG_LIMIT        0x78
#define PCI_CONFIG_REMAPBASE          0x90
#define PCI_CONFIG_REMAPLIMIT         0x98
#define PCI_CONFIG_TOM                0xa0
#define PCI_CONFIG_TOUUD              0xa8
#define PCI_CONFIG_BDSM               0xb0
#define PCI_CONFIG_BGSM               0xb4
#define PCI_CONFIG_TSEGMB             0xb8
#define PCI_CONFIG_TOLUD              0xbc

#define PCI_DMIBAR_DMI_UNCORRECTABLE_ERROR_STATUS 0x1c4

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

  EFI_STATUS status = EFI_SUCCESS;
  uint32_t id;
  uint8_t *dmibar;
  pci_device_addr addr = {0x00, 0x00, 0x00};
  uint64_t meseg_base, meseg_limit, remapbase, remaplimit, tom, touud;
  uint32_t dpr, tsegmb, tolud, bdsm, bgsm;

  pci_init();

  INFO("Experimental Intel host bridge driver initialization\n");

  id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);

  INFO("STATUS 0x%04x\n", pci_readw(id, PCI_CONFIG_STATUS));
  INFO("COMMAND 0x%04x\n", pci_readw(id, PCI_CONFIG_COMMAND));
  dmibar = (uint8_t*)(uintptr_t)pci_mm_readq(id, PCI_CONFIG_DMIBAR);
  INFO("COMMAND 0x%04x\n", pci_readw(id, PCI_CONFIG_COMMAND));
  struct dmiuests dmiuests = {*(uint32_t*)(dmibar +
      PCI_DMIBAR_DMI_UNCORRECTABLE_ERROR_STATUS)};
  print_dmiuests(&dmiuests);

  // Read registers #YOLO
  dpr = pci_mm_readd(id, PCI_CONFIG_DPR);
  meseg_base = pci_mm_readq(id, PCI_CONFIG_MESEG_BASE);
  meseg_limit = pci_mm_readq(id, PCI_CONFIG_MESEG_LIMIT);
  remapbase = pci_mm_readq(id, PCI_CONFIG_REMAPBASE);
  remaplimit = pci_mm_readq(id, PCI_CONFIG_REMAPLIMIT);
  tom = pci_mm_readq(id, PCI_CONFIG_TOM);
  touud = pci_mm_readq(id, PCI_CONFIG_TOUUD);
  bdsm = pci_mm_readd(id, PCI_CONFIG_BDSM);
  bgsm = pci_mm_readd(id, PCI_CONFIG_BGSM);
  tsegmb = pci_mm_readd(id, PCI_CONFIG_TSEGMB);
  tolud = pci_mm_readd(id, PCI_CONFIG_TOLUD);

  INFO("DPR(0x%08x)\n", dpr);
  INFO("MESEG_BASE(0x%016X)\n", meseg_base);
  INFO("MESEG_LIMIT(0x%016X)\n", meseg_limit);
  INFO("REMAPBASE(0x%016X)\n", remapbase);
  INFO("REMAPLIMIT(0x%016X)\n", remaplimit);
  INFO("TOM(0x%016X)\n", tom);
  INFO("TOUUD(0x%016X)\n", touud);
  INFO("BDSM(0x%08x)\n", bdsm);
  INFO("BGSM(0x%08x)\n", bgsm);
  INFO("TSEGMB(0x%08x)\n", tsegmb);
  INFO("TOLUD(0x%08x)\n", tolud);

  return status;
}
