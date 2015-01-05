#include "env.h"
#include "efi.h"
#include "efilib.h"
#include "stdio.h"
#include "env_flash.h"
#include "efi/efi_flash.h"

static protocol_flash *proto;

int env_flash_init(void) {
  EFI_STATUS status;
  EFI_GUID guid_flash = EFI_PROTOCOL_FLASH_GUID;

  status = LibLocateProtocol(&guid_flash, (void **)&proto);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }

  return ENV_OK;
}

int env_flash_call(struct registers *guest_regs) {
  return ENV_OK;
}

int env_flash_execute(void) {
  uint32_t dw;
  int ret;

  proto->cache_invalidate();
  // 0x437a is the location of default boot order
  // 1
  ret = proto->readd(proto->bios_base + 0x4374, &dw);
  if (ret) {
    INFO("Error while reading the flash...\n");
    return ENV_OK;
  }
  // INFO("Flash read : 0x%08x, ", dw);
  // printk("Init flash read : 0x%08x\n", *(uint32_t
  //       *)&proto->init_flash[0x4374]);
  // 2
  ret = proto->readd(proto->bios_base + 0x4378, &dw);
  if (ret) {
    INFO("Error while reading the flash...\n");
    return ENV_OK;
  }
  // INFO("Flash read : 0x%08x, ", dw);
  // printk("Init flash read : 0x%08x\n", *(uint32_t
  //       *)&proto->init_flash[0x4378]);
  // 3
  ret = proto->readd(proto->bios_base + 0x437c, &dw);
  if (ret) {
    INFO("Error while reading the flash...\n");
    return ENV_OK;
  }
  // INFO("Flash read : 0x%08x, ", dw);
  // printk("Init flash read : 0x%08x\n", *(uint32_t
  //       *)&proto->init_flash[0x437c]);
  // 4
  ret = proto->readd(proto->bios_base + 0x4380, &dw);
  if (ret) {
    INFO("Error while reading the flash...\n");
    return ENV_OK;
  }
  // INFO("Flash read : 0x%08x, ", dw);
  // printk("Init flash read : 0x%08x\n", *(uint32_t
  //       *)&proto->init_flash[0x4380]);
  // 5
  ret = proto->readd(proto->bios_base + 0x4384, &dw);
  if (ret) {
    INFO("Error while reading the flash...\n");
    return ENV_OK;
  }
  INFO("Flash read : 0x%08x, ", dw);
  printk("Init flash read : 0x%08x\n", *(uint32_t
        *)&proto->init_flash[0x4384]);

  return ENV_OK;
}
