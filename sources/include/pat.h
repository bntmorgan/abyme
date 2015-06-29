#ifndef __PAT_H__
#define __PAT_H__

#include <efi.h>
#include "types.h"

extern char *pat_type_strings[0x100];
#define PAT_TYPE_STRING(x) (pat_type_strings[x])

union pat_entries {
  uint64_t msr;
  struct {
    uint8_t pat0:3;
    uint8_t reserved0:5;
    uint8_t pat1:3;
    uint8_t reserved1:5;
    uint8_t pat2:3;
    uint8_t reserved2:5;
    uint8_t pat3:3;
    uint8_t reserved3:5;
    uint8_t pat4:3;
    uint8_t reserved4:5;
    uint8_t pat5:3;
    uint8_t reserved5:5;
    uint8_t pat6:3;
    uint8_t reserved6:5;
    uint8_t pat7:3;
    uint8_t reserved7:5;
  } __attribute__((packed));
  uint8_t entries[8];
} __attribute__((packed));

void pat_setup();

uint8_t pat_supported();

uint8_t pat_get_memory_type(uint64_t entry, uint8_t type);
uint8_t pat_set_memory_type(uint64_t *entry, uint8_t type, uint8_t mem_type);
uint8_t pat_set_memory_type_range(uint64_t address, uint8_t mem_type, uint64_t size);

#endif//__PAT_H__
