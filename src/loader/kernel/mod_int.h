#ifndef __MODS_INT_H__
#define __MODS_INT_H__

#include <stdint.h>

#include "include/mod.h"

#include "multiboot_int.h"

void mod_setup(mb_info_t *mb_info);

void mod_print_info(void);

void mod_reallocation(uint8_t *dest_addr);

uint32_t mod_get_size(void);

void mod_copy_info(mod_info_t *dest);

#endif
