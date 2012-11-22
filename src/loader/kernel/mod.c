#include "common/string_int.h"

#include "mod_int.h"

mod_info_t mod_info;

void mod_setup(mb_info_t *mb_info) {
  if (mb_info->mods_count != 1) {
    ERROR("bad number of modules\n");
  }
  mb_module_t *mb_modules = (mb_module_t *) mb_info->mods_addr;
  mod_info.mod_start = (uint32_t) mb_modules->mod_start;
  mod_info.mod_end = (uint32_t) mb_modules->mod_end;
  uint8_t j = 0;
  uint8_t *str = (uint8_t *) mb_modules->cmdline;
  while ((str[j] != 0) && j < (sizeof(mod_info.cmdline) / sizeof(mod_info.cmdline[0]) - 1)) {
    mod_info.cmdline[j] = str[j];
    j++;
  }
  mod_info.cmdline[j] = 0;
}

void mod_reallocation(uint8_t *dest_addr) {
  uint8_t *dest_addr_mod = dest_addr;
  INFO("reallocation of '%s' at %08x\n", mod_info.cmdline, (uint32_t) dest_addr_mod);
  INFO("entry point for '%s' at %08x\n", mod_info.cmdline, *((uint32_t *) (mod_info.mod_start + 28)));
  for (uint32_t j = mod_info.mod_start; j <= mod_info.mod_end; j++) {
    *dest_addr = *((uint8_t *) j);
    dest_addr++;
  }
  INFO("size: %08x\n", mod_info.mod_end - mod_info.mod_start + 1);
}

uint32_t mod_get_size(void) {
  return mod_info.mod_end - mod_info.mod_start + 1;
}

void mod_copy_info(mod_info_t *dest) {
  dest->mod_start = mod_info.mod_start;
  dest->mod_end = mod_info.mod_end;
  for (uint32_t i = 0; i < sizeof(mod_info.cmdline); i++) {
    dest->cmdline[i] = mod_info.cmdline[i];
  }
}

void mod_print_info(void) {
  INFO("start=%08x end=%08x cmdline='%s'\n",
      (uint32_t) mod_info.mod_start,
      (uint32_t) mod_info.mod_end,
      (uint8_t *) mod_info.cmdline);
}
