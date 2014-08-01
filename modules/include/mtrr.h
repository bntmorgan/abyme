#ifndef __MTRR_H__
#define __MTRR_H__

#include <efi.h>
#include "types.h"

#define MEMORY_TYPE_UC 0
#define MEMORY_TYPE_WC 1
#define MEMORY_TYPE_WT 4
#define MEMORY_TYPE_WP 5
#define MEMORY_TYPE_WB 6
#define MEMORY_TYPE_DEFAULT 255

#define MAX_NB_MTRR_VARIABLE	16
#define MAX_NB_MTRR_RANGES	  MAX_NB_MTRR_VARIABLE * 2 + 1

#define MTRR_NOT_SUPPORTED  1
#define MTRR_DISABLED       2

struct msr_mtrrcap {
  union {
    struct {
      uint64_t vcnt:8;
      uint64_t fix:1;
      uint64_t reserved_1:1;
      uint64_t wc:1;
      uint64_t smrr:1;
      uint64_t reserved_2:52;
    } __attribute__((packed));
    uint64_t value;
  } __attribute__((packed));
};

struct msr_mtrr_def_type {
  union {
    struct {
      uint64_t type:8;
      uint64_t reserved_1:2;
      uint64_t fe:1;
      uint64_t e:1;
      uint64_t reserved_2:52;
    } __attribute__((packed));
    uint64_t value;
  } __attribute__((packed));
};

struct msr_mtrr_fixed {
  struct {
    uint64_t address;
    uint64_t value;
  } msr;
  /* Computed part of the struct. */
  uint64_t range_address_begin;
  uint64_t range_address_end;
  uint64_t sub_range_size;
};

struct msr_mtrr_variable {
  struct {
    uint64_t address;
    union {
      struct {
        uint64_t type:8;
        uint64_t reserved_1:4;
        uint64_t phybase:52;
      } __attribute__((packed));
      uint64_t value;
    } __attribute__((packed));
  } msr_base;
  struct {
    uint64_t address;
    union {
      struct {
        uint64_t reserved_1:11;
        uint64_t valid:1;
        uint64_t phymask:52;
      } __attribute__((packed));
      uint64_t value;
    } __attribute__((packed));
  } msr_mask;
  /* Computed part of the struct. */
  uint64_t range_address_begin;
  uint64_t range_address_end;
};

struct memory_range {
  uint8_t type;
  uint64_t range_address_begin;
  uint64_t range_address_end;
};

void mtrr_print_ranges(void);

uint8_t mtrr_create_ranges(void);

inline uint8_t mtrr_get_nb_variable_mtrr(void);

const struct memory_range *mtrr_get_memory_range(uint64_t address);

#endif
