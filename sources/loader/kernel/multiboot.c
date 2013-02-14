/*
 * See [Multiboot_0.6.96].
 */

#include "multiboot.h"

#include "stdio.h"

multiboot_info_t *multiboot_info;
multiboot_module_t *multiboot_module;

void multiboot_setup(uint32_t magic, uint32_t *address) {
  multiboot_info = (multiboot_info_t *) address;
  if (magic != MB_BOOTLOADER_MAGIC) {
    ERROR("bad multiboot magic (get %x instead of %x)\n", magic, MB_BOOTLOADER_MAGIC);
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 3))) {
    ERROR("missing module information\n");
  }
  if (multiboot_info->mods_count != 1) {
    ERROR("bad number of modules\n");
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 6))) {
    ERROR("missing memory mapping information\n");
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 0))) {
    ERROR("missing memory information\n");
  }
  if (multiboot_info->mods_count != 1) {
    ERROR("bad number of modules\n");
  }
  multiboot_module = (multiboot_module_t *) multiboot_info->mods_addr;
}

void multiboot_print_info(void) {
  INFO("module start=%08x end=%08x cmdline='%s'\n",
      (uint32_t) multiboot_module->mod_start,
      (uint32_t) multiboot_module->mod_end,
      (uint8_t *) multiboot_module->cmdline);
}

multiboot_info_t *multiboot_get_info(void) {
  return multiboot_info;
}

uint32_t multiboot_get_module_start(void) {
  return multiboot_module->mod_start;
}
