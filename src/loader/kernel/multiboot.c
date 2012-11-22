#include "common/string_int.h"
#include "arch/cpu_int.h"

#include "multiboot_int.h"

mb_info_t *mb_info;
mb_module_t *mb_modules;

mb_info_t *mb_get_info(void) {
  return mb_info;
}

void mb_check(uint32_t magic, uint32_t *address) {
  mb_info = (mb_info_t *) address;
  if (magic != MB_BOOTLOADER_MAGIC) {
    ERROR("bad multiboot magic (get %x instead of %x)\n", magic, MB_BOOTLOADER_MAGIC);
  }
  if (! MB_CHECK_FLAG(mb_info->flags, (1 << 3))) {
    ERROR("missing module information\n");
  }
  if (mb_info->mods_count != 1) {
    ERROR("bad number of modules\n");
  }
  if (! MB_CHECK_FLAG(mb_info->flags, (1 << 6))) {
    ERROR("missing memory mapping information\n");
  }
  if (! MB_CHECK_FLAG(mb_info->flags, (1 << 0))) {
    ERROR("missing memory information\n");
  }
}
