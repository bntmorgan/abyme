#include "msr.h"
#include "cpuid.h"
#include "cpu.h"
#include "stdio.h"
#include "mtrr.h"

#define MEMORY_TYPE_UC 0
#define MEMORY_TYPE_WC 1
#define MEMORY_TYPE_WT 4
#define MEMORY_TYPE_WP 5
#define MEMORY_TYPE_WB 6

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

struct msr_mtrrcap {
  struct {
    uint64_t address;
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
  } msr;
};

struct msr_mtrr_def_type {
  struct {
    uint64_t address;
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
  } msr;
};

#define MAX_NB_MTRR_VARIABLE	16

struct memory_range memory_range[MAX_NB_MTRR_VARIABLE * 2 + 1];
uint8_t nb_memory_range;

struct memory_range memory_range_fixed[11];

struct msr_mtrr_variable mtrr_variable[MAX_NB_MTRR_VARIABLE];
struct msr_mtrr_fixed mtrr_fixed[11] = {
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX64K_00000}, 0x00000, 0x7ffff, 0x10000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX16K_80000}, 0x80000, 0x9ffff, 0x04000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX16K_A0000}, 0xa0000, 0xbffff, 0x04000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_C0000},  0xc0000, 0xc7fff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_C8000},  0xc8000, 0xcffff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_D0000},  0xd0000, 0xd7fff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_D8000},  0xd8000, 0xdffff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_E0000},  0xe0000, 0xe7fff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_E8000},  0xe8000, 0xeffff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_F0000},  0xf0000, 0xf7fff, 0x01000},
  {.msr= {.address=MSR_ADDRESS_IA32_MTRR_FIX4K_F8000},  0xf8000, 0xfffff, 0x01000},
};

uint8_t max_phyaddr;

struct msr_mtrrcap mtrr_cap;
struct msr_mtrr_def_type mtrr_def_type;

struct memory_range default_memory_range = {MEMORY_TYPE_UC, 0, 0xffffffffffffffff, NULL};

uint8_t check_memory_type(uint8_t type) {
  if ((type != 0) && (type != 1) && (type != 4) && (type != 5) && (type != 6)) {
    panic("!#MTRR TYPE [%d]", type);
  }
  return type;
}

uint8_t mtrr_initialize(void) {
  if (cpuid_are_mtrr_supported() == 1) {
    uint8_t i;
    mtrr_cap.msr.address = MSR_ADDRESS_IA32_MTRRCAP;
    mtrr_cap.msr.value = msr_read(MSR_ADDRESS_IA32_MTRRCAP);
    mtrr_def_type.msr.address = MSR_ADDRESS_A32_MTRR_DEF_TYPE;
    mtrr_def_type.msr.value = msr_read(MSR_ADDRESS_A32_MTRR_DEF_TYPE);
    /* Retrieve fixed MTRRs corresponding to the range 0x000000:0x100000. */
    for (i = 0; i < 11; i++) {
      mtrr_fixed[i].msr.value = msr_read(mtrr_fixed[i].msr.address);
    }
    /* Retrieve variable MTRRs. */
    if (mtrr_cap.msr.vcnt > MAX_NB_MTRR_VARIABLE) {
      panic("!#MTRR VCNT [%d]", mtrr_cap.msr.vcnt);
    }
    uint64_t msr_base_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0;
    uint64_t msr_mask_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0 + 1;
    max_phyaddr = cpuid_get_maxphyaddr();
    for (i = 0; i < mtrr_cap.msr.vcnt; i++) {
      mtrr_variable[i].msr_base.address = msr_base_address;
      mtrr_variable[i].msr_mask.address = msr_mask_address;
      mtrr_variable[i].msr_base.value = msr_read(mtrr_variable[i].msr_base.address);
      mtrr_variable[i].msr_mask.value = msr_read(mtrr_variable[i].msr_mask.address);
      mtrr_variable[i].range_address_begin = ((uint64_t) mtrr_variable[i].msr_base.phybase) << 12;
      uint64_t mask = ((uint64_t) mtrr_variable[i].msr_mask.phymask) << 12;
      uint64_t len = (~(mask & 0xfffffffffffff000)) & (((uint64_t) 1 << max_phyaddr) - 1);
      mtrr_variable[i].range_address_end = mtrr_variable[i].range_address_begin + len;
      msr_base_address += 2;
      msr_mask_address += 2;
    }
  }
  return 0;
}

uint8_t mtrr_compute_memory_ranges(void) {
  uint8_t i;
  uint8_t j;
  uint8_t k;
  /* Compute for fixed MTRR. */
  /* k is the index of the range being computed. */
  /* i and j are indexes used to swap through the fixed MTRR. */
  k = 0;
  memory_range[k].type = mtrr_fixed[0].msr.value & 0xff;
  memory_range_fixed[k].range_address_begin = 0;
  memory_range_fixed[k].range_address_end = mtrr_fixed[0].sub_range_size - 1;
  memory_range_fixed[k].next = NULL;
  for (i = 0; i < 11; i++) {
    for (j = 0; j < 8; j++) {
      if (memory_range[k].type == (mtrr_fixed[i].msr.value >> (j * 8)) & 0xff) {
        memory_range_fixed[k].range_address_end += mtrr_fixed[i].sub_range_size;
      } else {
        memory_range_fixed[k].next = &memory_range_fixed[k + 1];
        k++;
        memory_range[k].type = (mtrr_fixed[i].msr.value >> (j * 8)) & 0xff;
        memory_range_fixed[k].range_address_begin = memory_range_fixed[k + 1].range_address_end + 1;
        memory_range_fixed[k].range_address_end = mtrr_fixed[i].sub_range_size - 1;
        memory_range_fixed[k].next = NULL;
      }
    }
  }
  /* Compute for variable MTRR. */
  memory_range[0].type = 0;
  memory_range[0].range_address_begin = 0x0000000000000000;
  memory_range[0].range_address_end = 0xffffffffffffffff;
  memory_range[0].next = NULL;
  nb_memory_range = 0;
  /* Split all the memory according to MTRR. */
  for (i = 0; i < mtrr_cap.msr.vcnt; i++) {
    if (mtrr_variable[i].msr_mask.valid == 1) {
      struct memory_range *ptr = &memory_range[0];
      uint64_t mtrr_address_begin = mtrr_variable[i].range_address_begin;
      uint64_t mtrr_address_end = mtrr_variable[i].range_address_end;
      uint64_t mtrr_type = mtrr_variable[i].msr_base.type;
      while (ptr != NULL && mtrr_address_begin <= mtrr_address_end) {
        /* MTTR considered is either after the current range or in this range.
         * It can't be before this range, otherwise we miss this situation in
         * the previous iteration.
         */
        if (mtrr_address_begin == ptr->range_address_begin) {
          if (mtrr_address_end < ptr->range_address_end) {
            nb_memory_range = nb_memory_range + 1;
            memory_range[nb_memory_range].next = ptr->next;
            memory_range[nb_memory_range].type = ptr->type;
            memory_range[nb_memory_range].range_address_begin = mtrr_address_end + 1;
            memory_range[nb_memory_range].range_address_end = ptr->range_address_end;
            ptr->range_address_end = mtrr_address_end;
            ptr->next = &memory_range[nb_memory_range];
          }
          ptr->type |= (1 << check_memory_type(mtrr_type));
          mtrr_address_begin = ptr->range_address_end + 1;
        } else if (mtrr_address_begin <= ptr->range_address_end) {
          nb_memory_range = nb_memory_range + 1;
          memory_range[nb_memory_range].next = ptr->next;
          memory_range[nb_memory_range].type = ptr->type;
          memory_range[nb_memory_range].range_address_begin = mtrr_address_begin;
          memory_range[nb_memory_range].range_address_end = ptr->range_address_end;
          ptr->range_address_end = mtrr_address_begin - 1;
          ptr->next = &memory_range[nb_memory_range];
        }
        ptr = ptr->next;
      }
    }
  }
  /* Get the type of each range (overwrite type of ranges!). */
  struct memory_range *ptr = &memory_range[0];
  while (ptr != NULL) {
    if (ptr->type == 0x0) {
      ptr->type = mtrr_def_type.msr.type;
    } else if ((ptr->type & (1 << MEMORY_TYPE_UC)) != 0x0) {
      ptr->type = MEMORY_TYPE_UC;
    } else if (ptr->type == (1 << MEMORY_TYPE_WC)) {
      ptr->type = MEMORY_TYPE_WC;
    } else if (ptr->type == (1 << MEMORY_TYPE_WT)) {
      ptr->type = MEMORY_TYPE_WT;
    } else if (ptr->type == (1 << MEMORY_TYPE_WP)) {
      ptr->type = MEMORY_TYPE_WP;
    } else if (ptr->type == (1 << MEMORY_TYPE_WB)) {
      ptr->type = MEMORY_TYPE_WB;
    } else if (ptr->type == ((1 << MEMORY_TYPE_WB) | (1 << MEMORY_TYPE_WT))) {
      ptr->type = MEMORY_TYPE_WT;
    } else {
      panic("#MTRR MT [%d]", ptr->type);
    }
    ptr = ptr->next;
  }
  /* Merge range if possible. */
  ptr = &memory_range[0];
  while (ptr->next != NULL) {
    if (ptr->type == ptr->next->type) {
      ptr->range_address_end = ptr->next->range_address_end;
      ptr->next = ptr->next->next;
    } else {
      ptr = ptr->next;
    }
  }
  return 0;
}

uint8_t mtrr_get_nb_variable_mtrr(void) {
  return mtrr_cap.msr.vcnt;
}

const struct memory_range *mtrr_get_memory_range(uint64_t address) {
  if (cpuid_are_mtrr_supported() == 1 && mtrr_def_type.e == 1) {
    struct memory_range *ptr = &memory_range[0];
    uint8_t i;
    if (address < 0x100000 && mtrr_cap.fix == 1 && mtrr_def_type.fe == 1) {
      ptr = &memory_range_fixed[0];
    }
    while (ptr->range_address_begin > address) {
      ptr = ptr->next;
    }
    return ptr;
  }
  return &default_memory_range;
}

void mtrr_print_ranges(void) {
  struct memory_range *ptr = &memory_range_fixed[0];
  while (ptr != NULL) {
    printk("F 0x%016X-0x%016X %d\n", ptr->range_address_begin, ptr->range_address_end, ptr->type);
    ptr = ptr->next;
  }
  ptr = &memory_range[0];
  while (ptr != NULL) {
    printk("V 0x%016X-0x%016X %d\n", ptr->range_address_begin, ptr->range_address_end, ptr->type);
    ptr = ptr->next;
  }
}
