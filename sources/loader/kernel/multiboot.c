/*
 * See [Multiboot_0.6.96].
 */

#include "multiboot.h"

#include "stdio.h"

multiboot_info_t *multiboot_info;
multiboot_module_t *multiboot_modules[2];

void multiboot_setup(uint32_t magic, uint32_t *address) {
  multiboot_info = (multiboot_info_t *) address;
  if (magic != MB_BOOTLOADER_MAGIC) {
    ERROR("bad multiboot magic (get %x instead of %x)\n", magic, MB_BOOTLOADER_MAGIC);
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 3))) {
    ERROR("missing module information\n");
  }
  if (multiboot_info->mods_count != 2) {
    ERROR("bad number of modules: loader vmm rm_kernel\n");
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 6))) {
    ERROR("missing memory mapping information\n");
  }
  if (! MB_CHECK_FLAG(multiboot_info->flags, (1 << 0))) {
    ERROR("missing memory information\n");
  }
/*TODO: why twice??
  if (multiboot_info->mods_count != 1) {
    ERROR("bad number of modules\n");
  }
*/
  multiboot_modules[0] = (multiboot_module_t *) multiboot_info->mods_addr;
  multiboot_modules[1] = multiboot_modules[0] + 1;
}

void multiboot_print_info(void) {
  INFO("module start=%08x end=%08x cmdline='%s'\n",
      (uint32_t) multiboot_modules[0]->mod_start,
      (uint32_t) multiboot_modules[0]->mod_end,
      (uint8_t *) multiboot_modules[0]->cmdline);
  if (multiboot_modules[1] != 0) {
    INFO("module start=%08x end=%08x cmdline='%s'\n",
        (uint32_t) multiboot_modules[0]->mod_start,
        (uint32_t) multiboot_modules[0]->mod_end,
        (uint8_t *) multiboot_modules[0]->cmdline);
  }
}

multiboot_info_t *multiboot_get_info(void) {
  return multiboot_info;
}

uint32_t multiboot_get_module_end(uint8_t index) {
  return multiboot_modules[index]->mod_end;
}

uint32_t multiboot_get_module_start(uint8_t index) {
  return multiboot_modules[index]->mod_start;
}
