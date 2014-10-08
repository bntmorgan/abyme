#include <efi.h>
#include <efilib.h>

#include "pci.h"
#include "efi/efi_eric.h"
#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "efiw.h"

// 
// stdio putc pointer
//
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void disable_debug(void){
  putc = &put_nothing;
}

enum bbs_type {
  LPC,
  RES,
  PCI,
  SPI
};

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  EFI_STATUS status = EFI_SUCCESS;
  uint8_t wait, loop_limit;
  uint16_t hsfs, hsfs_fdv, data_from;
  uint32_t rcba, rcrb, gcs, bbs, frap, freg1, bios_base, bios_limit, index,
           size_to_read, faddr, hsfc, i, data;
  uint64_t mmio_base, device_base;
  uint32_t *buf;

  pci_device_addr addr_lpc;
  if (pci_get_device(0x8086, 0x8C56, &addr_lpc)) { // C226 LPC SKU
    return EFI_NOT_FOUND;
  }

  INFO("Experimental SPI flash driver initialization\n");

  mmio_base = pci_mmio_get_base(); 
  INFO("MMIO base : 0x%016X\n", mmio_base);

  device_base = mmio_base | PCI_MAKE_MMCONFIG(addr_lpc.bus, addr_lpc.device,
      addr_lpc.function);
  INFO("Device base : 0x%016X\n", device_base);

  // See 12.1.40 of Intel ® 8 Series/C220 Series Chipset Family Platform
  // Controller Hub (PCH).
  // Root Complex Register Block address
  rcba = *(uint32_t*)(device_base | 0xf0);
  INFO("RCBA = 0x%X\n", rcba);

  rcrb = rcba & 0xffffc000;

  // 10.1.49
  gcs = *(uint32_t *)(uintptr_t)(rcrb | 0x3410);
  bbs = ((gcs >> 10) & 0x3);
  INFO("GCS = 0x%08x, BBS 0x%08x\n", gcs, bbs);

  if (bbs != SPI) {
    return EFI_UNSUPPORTED;
  }
  INFO("BOOT BIOS Straps is SPI\n");

  // See 21.1:
  // The SPI Host Interface registers are memory-mapped in the RCRB (Root
  // Complex Register Block) Chipset Register Space with a base address (SPIBAR)
  // of 3800h and are located within the range of 3800h to 39FFh.

  // Test "flash descriptor valid"
  // TODO: test lockdown ???
  hsfs = *(uint16_t *)(uintptr_t)(rcrb | 0x3804);
  hsfs_fdv = hsfs & (1 << 14);
  INFO("HSFS: 0x%02x\n", hsfs);
  INFO("HSFS_fdv: 0x%02x\n", hsfs_fdv);

  if (hsfs_fdv == 0) {
    INFO("flash descriptor invalid!\n");
    return EFI_UNSUPPORTED;
  }

  // See 21.1.9: region 1 is BIOS
  frap = *(uint32_t *)(uintptr_t)(rcrb | 0x3850);
  INFO("FRAP: 0x%08x\n",frap);
  freg1 = *(uint32_t *)(uintptr_t)(rcrb | 0x3858);
  INFO("FREG1: 0x%08x\n", freg1);
  bios_base = ((freg1 >> 0) & 0xfff) << 12;
  bios_limit = (((freg1 >> 16) & 0xfff) << 12) | 0xfff;
  INFO("BIOS base: 0x%08x  limit: 0x%08x\n", bios_base, bios_limit);

  // Allocating memory
  buf = efi_allocate_pages((bios_limit - bios_base) / 0x1000);
  if (!buf) {
    INFO("Failed to allocate buffer\n");
    return EFI_OUT_OF_RESOURCES;
  }
  INFO("Buffer at : 0x%016X (0x%x pages), size : 0x%08x\n", buf, (bios_limit -
        bios_base + 1) / 0x1000, (bios_limit - bios_base + 1));

  // See 21.1.2: FDONE, FCERR, AEL must be cleared (i.e. 0). If they equal 1, we
  // can write this value for a reset.  
  // REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));
  *(uint16_t *)(uintptr_t)(rcrb | 0x3804) = hsfs;

  index = bios_base;
  while (index < bios_limit) {
    // INFO("Processing : 0x%08x\n", index);
    size_to_read = MIN(64, bios_limit - index + 1); 

    // See 21.1.4
    faddr = (*(uint32_t *)(uintptr_t)(rcrb | 0x3808) & 0xfe000000) | index;
	  *(uint32_t *)(uintptr_t)(rcrb | 0x3808) = faddr;

    // See 21.1.3
    hsfc = *(uint16_t *)(uintptr_t)(rcrb | 0x3806);
    hsfc = hsfc & 0xc0f9; // Clear bits 2:1 (0x___9) => we want a read operation.
                          // Clear bits 13:8 (0xc0__) => prepare byte count
    hsfc = hsfc | (((size_to_read - 1) << 8) & 0x3f00) | 1; // Add byte count
                                                            // and GO
    *(uint16_t *)(uintptr_t)(rcrb | 0x3806) = hsfc;


    // Wait for the end of read...
    // See FDONE and FCERR in 21.1.2
    wait = 1;
    loop_limit = 32;
    while (wait) {
      hsfs = *(uint16_t *)(uintptr_t)(rcrb | 0x3804);
      wait = ((hsfs & 0x3) == 0);
      if (wait) {
        for (i = 0; i < 10; i++);
      }
      loop_limit -= 1;
      if (loop_limit == 0) {
        INFO("Loop limit at 0x%08x\n", index);
        return EFI_COMPROMISED_DATA;
      }
    }
    if ((hsfs & 0x2) == 0x2) {
      INFO("Error at 0x%08x\n", index);
      return EFI_COMPROMISED_DATA;
    }
    // Reset FDONE and FCERR
    *(uint16_t *)(uintptr_t)(rcrb | 0x3804) = *(uint16_t *)(uintptr_t)(rcrb |
        0x3804);

    for (i = 0; i < size_to_read / 4; i++) {
      // See 21.2.5
      data_from = 0x3810 + 4 * i;
      data = *(uint32_t *)(uintptr_t)(rcrb | data_from);
      // INFO("DATA : 0x08x\n", data);
      buf[index + i] = data;
    }
    // It is impossible to replace the previous loop by:
    //    fout.write(rcrb[0x3810:(0x3810 + size_to_read)])
    // Maybe we can't read several register at the same time!

    index += 64;
    // break;
  }

  return status;
}
