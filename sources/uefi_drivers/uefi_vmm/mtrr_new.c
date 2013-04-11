#include "hardware/msr.h"
#include "hardware/cpuid.h"
#include "stdio.h"
#include "mtrr.h"

#define MEMORY_TYPE_UC 0
#define MEMORY_TYPE_WC 1
#define MEMORY_TYPE_WT 4
#define MEMORY_TYPE_WP 5
#define MEMORY_TYPE_WB 6

#define panic ERROR

uint8_t mtrr_support(void) {
  uint64_t rdx;
  __asm__ __volatile__(
    "cpuid;"
  : "=d"(rdx) : "a"(0x1));
  INFO("rdx : %x, bit 12 support ? %d\n", rdx, (rdx >> 12) & 0x1);
  return (rdx >> 12) & 0x1;
}

struct msr_mtrr_fixed {
  struct {
    uint64_t address;
    union {
      struct {
        uint64_t range_1:8;
        uint64_t range_2:8;
        uint64_t range_3:8;
        uint64_t range_4:8;
        uint64_t range_5:8;
        uint64_t range_6:8;
        uint64_t range_7:8;
        uint64_t range_8:8;
      } __attribute__((packed));
      uint64_t value;
    } __attribute__((packed));
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

struct memory_range {
  union {
    struct {
      uint8_t uc;//:1;
      uint8_t wc;//:1;
      uint8_t wt;//:1;
      uint8_t wp;//:1;
      uint8_t wb;//:1;
      //uint8_t reserved_1:3;
    } __attribute__((packed));
    uint8_t type;
  } __attribute__((packed));
  uint64_t range_address_begin;
  uint64_t range_address_end;
  struct memory_range *next;
};

#define MAX_NB_MTRR_VARIABLE	16

struct memory_range memory_range[MAX_NB_MTRR_VARIABLE * 2 + 1];
uint8_t last_memory_range_used;

struct msr_mtrr_variable mtrr_variable[MAX_NB_MTRR_VARIABLE];
struct msr_mtrr_fixed mtrr_fixed_todo[11];

uint8_t max_phyaddr;

struct msr_mtrrcap mtrr_cap;
struct msr_mtrr_def_type mtrr_def_type;

uint8_t check_memory_type(uint8_t type) {
  if ((type != 0) && (type != 1) && (type != 4) && (type != 5) && (type != 6)) {
    panic("Bad memory type: %02x\n", type);
  }
  return type;
}

uint8_t mtrr_initialize(void) {
  printk("AAA\n");
  if (mtrr_support() == 1) {
    mtrr_cap.msr.address = MSR_ADDRESS_IA32_MTRRCAP;
    mtrr_cap.msr.value = msr_read64(MSR_ADDRESS_IA32_MTRRCAP);
    mtrr_def_type.msr.address = MSR_ADDRESS_A32_MTRR_DEF_TYPE;
    mtrr_def_type.msr.value = msr_read64(MSR_ADDRESS_A32_MTRR_DEF_TYPE);
    /* Retrieve fixed MTRRs corresponding to the range 0x000000:0x100000. */
    mtrr_fixed_todo[0].msr.address = MSR_ADDRESS_IA32_MTRR_FIX64K_00000;
    mtrr_fixed_todo[0].msr.value = msr_read64(mtrr_fixed_todo[0].msr.address);
    mtrr_fixed_todo[0].sub_range_size = 0x10000;
    mtrr_fixed_todo[0].range_address_begin = 0x00000;
    mtrr_fixed_todo[0].range_address_end = mtrr_fixed_todo[0].range_address_begin + mtrr_fixed_todo[0].sub_range_size * 8;
    mtrr_fixed_todo[1].msr.address = MSR_ADDRESS_IA32_MTRR_FIX16K_80000;
    mtrr_fixed_todo[1].msr.value = msr_read64(mtrr_fixed_todo[1].msr.address);
    mtrr_fixed_todo[1].sub_range_size = 0x4000;
    mtrr_fixed_todo[1].range_address_begin = 0x80000;
    mtrr_fixed_todo[1].range_address_end = mtrr_fixed_todo[1].range_address_begin + mtrr_fixed_todo[1].sub_range_size * 8;
    mtrr_fixed_todo[2].msr.address = MSR_ADDRESS_IA32_MTRR_FIX16K_A0000;
    mtrr_fixed_todo[2].msr.value = msr_read64(mtrr_fixed_todo[2].msr.address);
    mtrr_fixed_todo[2].sub_range_size = 0x4000;
    mtrr_fixed_todo[2].range_address_begin = 0xa0000;
    mtrr_fixed_todo[2].range_address_end = mtrr_fixed_todo[2].range_address_begin + mtrr_fixed_todo[2].sub_range_size * 8;
    mtrr_fixed_todo[3].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_C0000;
    mtrr_fixed_todo[3].msr.value = msr_read64(mtrr_fixed_todo[3].msr.address);
    mtrr_fixed_todo[3].sub_range_size = 0x1000;
    mtrr_fixed_todo[3].range_address_begin = 0xc0000;
    mtrr_fixed_todo[3].range_address_end = mtrr_fixed_todo[3].range_address_begin + mtrr_fixed_todo[3].sub_range_size * 8;
    mtrr_fixed_todo[4].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_C8000;
    mtrr_fixed_todo[4].msr.value = msr_read64(mtrr_fixed_todo[4].msr.address);
    mtrr_fixed_todo[4].sub_range_size = 0x1000;
    mtrr_fixed_todo[4].range_address_begin = 0xc8000;
    mtrr_fixed_todo[5].range_address_end = mtrr_fixed_todo[4].range_address_begin + mtrr_fixed_todo[4].sub_range_size * 8;
    mtrr_fixed_todo[5].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_D0000;
    mtrr_fixed_todo[5].msr.value = msr_read64(mtrr_fixed_todo[5].msr.address);
    mtrr_fixed_todo[5].sub_range_size = 0x1000;
    mtrr_fixed_todo[5].range_address_begin = 0xd0000;
    mtrr_fixed_todo[5].range_address_end = mtrr_fixed_todo[5].range_address_begin + mtrr_fixed_todo[5].sub_range_size * 8;
    mtrr_fixed_todo[6].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_D8000;
    mtrr_fixed_todo[6].msr.value = msr_read64(mtrr_fixed_todo[6].msr.address);
    mtrr_fixed_todo[6].sub_range_size = 0x1000;
    mtrr_fixed_todo[6].range_address_begin = 0xd8000;
    mtrr_fixed_todo[6].range_address_end = mtrr_fixed_todo[6].range_address_begin + mtrr_fixed_todo[6].sub_range_size * 8;
    mtrr_fixed_todo[7].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_E0000;
    mtrr_fixed_todo[7].msr.value = msr_read64(mtrr_fixed_todo[7].msr.address);
    mtrr_fixed_todo[7].sub_range_size = 0x1000;
    mtrr_fixed_todo[7].range_address_begin = 0xe0000;
    mtrr_fixed_todo[7].range_address_end = mtrr_fixed_todo[7].range_address_begin + mtrr_fixed_todo[7].sub_range_size * 8;
    mtrr_fixed_todo[8].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_E8000;
    mtrr_fixed_todo[8].msr.value = msr_read64(mtrr_fixed_todo[8].msr.address);
    mtrr_fixed_todo[8].sub_range_size = 0x1000;
    mtrr_fixed_todo[8].range_address_begin = 0xe8000;
    mtrr_fixed_todo[8].range_address_end = mtrr_fixed_todo[8].range_address_begin + mtrr_fixed_todo[8].sub_range_size * 8;
    mtrr_fixed_todo[9].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_F0000;
    mtrr_fixed_todo[9].msr.value = msr_read64(mtrr_fixed_todo[9].msr.address);
    mtrr_fixed_todo[9].sub_range_size = 0x1000;
    mtrr_fixed_todo[9].range_address_begin = 0xf0000;
    mtrr_fixed_todo[9].range_address_end = mtrr_fixed_todo[9].range_address_begin + mtrr_fixed_todo[9].sub_range_size * 8;
    mtrr_fixed_todo[10].msr.address = MSR_ADDRESS_IA32_MTRR_FIX4K_F8000;
    mtrr_fixed_todo[10].msr.value = msr_read64(mtrr_fixed_todo[10].msr.address);
    mtrr_fixed_todo[10].sub_range_size = 0x1000;
    mtrr_fixed_todo[10].range_address_begin = 0xf8000;
    mtrr_fixed_todo[10].range_address_end = mtrr_fixed_todo[10].range_address_begin + mtrr_fixed_todo[10].sub_range_size * 8;
  printk("BBB\n");
    /* Retrieve variable MTRRs. */
    if (mtrr_cap.msr.vcnt > MAX_NB_MTRR_VARIABLE) {
      panic("#MTRR [%d]", mtrr_cap.msr.vcnt);
    }
  printk("CCC\n");
    uint64_t msr_base_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0;
    uint64_t msr_mask_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0 + 1;
    max_phyaddr = cpuid_get_maxphyaddr();
    uint8_t i = 0;
    for (i = 0; i < mtrr_cap.msr.vcnt; i++) {
      mtrr_variable[i].msr_base.address = msr_base_address;
      mtrr_variable[i].msr_mask.address = msr_mask_address;
      mtrr_variable[i].msr_base.value = msr_read64(mtrr_variable[i].msr_base.address);
      mtrr_variable[i].msr_mask.value = msr_read64(mtrr_variable[i].msr_mask.address);
      mtrr_variable[i].range_address_begin = ((uint64_t) mtrr_variable[i].msr_base.phybase) << 12;
      uint64_t mask = ((uint64_t) mtrr_variable[i].msr_mask.phymask) << 12;
      uint64_t len = (~(mask & 0xfffffffffffff000)) & (((uint64_t) 1 << max_phyaddr) - 1);
      mtrr_variable[i].range_address_end = mtrr_variable[i].range_address_begin + len;
      msr_base_address += 2;
      msr_mask_address += 2;
    }
  printk("DDD\n");
  }
  return 0;
}

void mtrr_add_memory_range(uint8_t type, struct memory_range *memory_range) {
  type = check_memory_type(type);
  if (type == MEMORY_TYPE_UC) {
    memory_range->uc = 1;
  } else if (type == MEMORY_TYPE_WC) {
    memory_range->wc = 1;
  } else if (type == MEMORY_TYPE_WT) {
    memory_range->wt = 1;
  } else if (type == MEMORY_TYPE_WP) {
    memory_range->wc = 1;
  } else if (type == MEMORY_TYPE_WB) {
    memory_range->wb = 1;
  }
}

uint8_t mtrr_compute_memory_ranges(void) {
  printk("EEE\n");
  memory_range[0].type = 0;
  memory_range[0].range_address_begin = 0x0000000000000000;
  memory_range[0].range_address_end = 0xffffffffffffffff;
  memory_range[0].next = NULL;
  mtrr_add_memory_range(mtrr_def_type.msr.type, &memory_range[0]);
  last_memory_range_used = 0;
  /* Split the memory according to MTRR. */
  uint8_t i;
  for (i = 0; i < mtrr_cap.msr.vcnt; i++) {
  printk("FFF\n");
//print_ranges();
    struct memory_range *ptr = &memory_range[0];
    uint64_t mtrr_address_begin = mtrr_variable[i].range_address_begin;
    uint64_t mtrr_address_end = mtrr_variable[i].range_address_end;
    uint64_t mtrr_type = mtrr_variable[i].msr_base.type;
    printk("processing %x %x %d\n", mtrr_address_begin, mtrr_address_end, mtrr_type);
    while (ptr != NULL && mtrr_address_begin <= mtrr_address_end) {
 //   printk("           %x %x %d\n", mtrr_address_begin, mtrr_address_end, mtrr_type);
      /* MTTR considered is either after the current range or in this range.
       * It can't be before this range, otherwise we miss this situation in
       * the previous iteration.
       */
      if (mtrr_address_begin == ptr->range_address_begin) {
        if (mtrr_address_end < ptr->range_address_end) {
          last_memory_range_used = last_memory_range_used + 1;
          memory_range[last_memory_range_used].next = ptr->next;
          memory_range[last_memory_range_used].type = ptr->type;
          memory_range[last_memory_range_used].range_address_begin = mtrr_address_end + 1;
          memory_range[last_memory_range_used].range_address_end = ptr->range_address_end;
          ptr->range_address_end = mtrr_address_end;
          ptr->next = &memory_range[last_memory_range_used];
        }
        mtrr_add_memory_range(mtrr_type, ptr);
        mtrr_address_begin = ptr->range_address_end + 1;
      } else if (mtrr_address_begin <= ptr->range_address_end) {
        last_memory_range_used = last_memory_range_used + 1;
        memory_range[last_memory_range_used].next = ptr->next;
        memory_range[last_memory_range_used].type = ptr->type;
        memory_range[last_memory_range_used].range_address_begin = mtrr_address_begin;
        memory_range[last_memory_range_used].range_address_end = ptr->range_address_end;
        ptr->range_address_end = mtrr_address_begin - 1;
        ptr->next = &memory_range[last_memory_range_used];
      }
      ptr = ptr->next;
    }
  }
  printk("GGG\n");
  /* Get the type of each range (overwrite type of ranges!). */
  struct memory_range *ptr = &memory_range[0];
  while (ptr != NULL) {
    if (ptr->uc == 1) {
      ptr->type = MEMORY_TYPE_UC;
    } else if (ptr->type == 0x2) {
      ptr->type = MEMORY_TYPE_WC;
    } else if (ptr->type == 0x4) {
      ptr->type = MEMORY_TYPE_WT;
    } else if (ptr->type == 0x8) {
      ptr->type = MEMORY_TYPE_WP;
    } else if (ptr->type == 0x10) {
      ptr->type = MEMORY_TYPE_WB;
    } else if (ptr->type == 0x14) {
      ptr->type = MEMORY_TYPE_WT;
    } else {
      panic("#MTRR MT [%d]", ptr->type);
    }
    ptr = ptr->next;
  }
print_ranges();
  printk("HHH\n");
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
  printk("III\n");
  return 0;
}

void print_ranges(void) {
  struct memory_range *ptr = &memory_range[0];
  printk("JJJ\n");
  while (ptr != NULL) {
    printk("0x%016X-0x%016X %d\n", ptr->range_address_begin, ptr->range_address_end, ptr->type);
    ptr = ptr->next;
  }
  printk("KKK\n");
}

void mtrr_save(void) {
  /* TODO: save state of MTRR */
}
