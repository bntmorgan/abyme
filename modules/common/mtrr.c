#include "msr.h"
#include "cpuid.h"
#include "cpu.h"
#include "stdio.h"
#include "mtrr.h"
#include "include/string.h"

struct memory_range memory_range[MAX_NB_MTRR_RANGES];
uint8_t nb_memory_range;

/* MSR MTRR corresponding structures */
struct msr_mtrrcap mtrr_cap;
struct msr_mtrr_def_type mtrr_def_type;
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
struct msr_mtrr_variable mtrr_variable[MAX_NB_MTRR_VARIABLE];

/* Store pointers to variables MTRRs sorted by begin_addr */
struct msr_mtrr_variable* sorted_mtrr_variable[MAX_NB_MTRR_VARIABLE];
uint8_t nb_enabled_var_mtrr;


static void compute_var_MTRR(uint8_t *k, uint64_t *current_addr, uint8_t begin_index, uint8_t type, uint64_t max_addr);
static void fill_gap(uint8_t *k, uint64_t *current_addr, uint64_t to, uint8_t type);

inline static uint64_t min(uint64_t a, uint64_t b) {
  return (a<b)?a:b;
}

/* Compares two variable MTRRs */
inline static uint8_t is_inf(struct msr_mtrr_variable* a, struct msr_mtrr_variable* b) {
  return (a->range_address_begin < b->range_address_begin);
}

/* Return the final type in case of type conflict */
static uint8_t get_type(uint8_t t1, uint8_t t2) {
  if (t1 == MEMORY_TYPE_DEFAULT || t2 == MEMORY_TYPE_DEFAULT) {
    return (t1 == MEMORY_TYPE_DEFAULT)?t2:t1;
  } else if (t1 == t2) {
    return t1;
  } else if (t1 == MEMORY_TYPE_UC || t2 == MEMORY_TYPE_UC) {
    return MEMORY_TYPE_UC;
  } else if ( (t1 == MEMORY_TYPE_WB && t2 == MEMORY_TYPE_WT)
            ||(t1 == MEMORY_TYPE_WT && t2 == MEMORY_TYPE_WB)) {
    return MEMORY_TYPE_WT;
  } else {
    // undefinied behaviour
    panic("MTRR overlaped memory : Undefined behaviour, t1=%d, t2=%d\n", t1, t2);
    return -1;
  }
}

inline static uint8_t real_type(uint8_t type) {
  if (type == MEMORY_TYPE_DEFAULT) {
    return mtrr_def_type.type;
  } else {
    return type;
  }
}

static void check_memory_type(uint8_t type) {
  if ((type != MEMORY_TYPE_UC) && (type != MEMORY_TYPE_WC) && (type != MEMORY_TYPE_WT)
      && (type != MEMORY_TYPE_WP) && (type != MEMORY_TYPE_WB)) {
    panic("!#MTRR TYPE [%d]\n", type);
  }
}

uint8_t mtrr_create_ranges(void) {
  uint8_t i;
  uint8_t j;
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
  uint64_t last_addr = ((uint64_t) 1 << max_phyaddr) - 1;

  /* Check if the processor support MTRRs */
  if (cpuid_are_mtrr_supported() != 1) {
    memory_range[0].type=MEMORY_TYPE_UC;
    memory_range[0].range_address_begin=0;
    memory_range[0].range_address_end=last_addr;
    nb_memory_range=1;
    return MTRR_NOT_SUPPORTED;
  }

  /* Retrieve MTRR conf registers */
  mtrr_cap.value = msr_read(MSR_ADDRESS_IA32_MTRRCAP);
  mtrr_def_type.value = msr_read(MSR_ADDRESS_IA32_MTRR_DEF_TYPE);

  /* If MTRRs are not enabled we can stop here */
  if (!mtrr_def_type.e) {
    memory_range[0].type=MEMORY_TYPE_UC;
    memory_range[0].range_address_begin=0;
    memory_range[0].range_address_end=last_addr;
    nb_memory_range=1;
    return MTRR_DISABLED;
  }

  /* Retrieve fixed MTRRs corresponding to the range 0x000000:0x100000. */
  if (mtrr_cap.fix) {
    for (i = 0; i < 11; i++) {
      mtrr_fixed[i].msr.value = msr_read(mtrr_fixed[i].msr.address);
    }
  }

  /* Retrieve variable MTRRs. */
  if (mtrr_cap.vcnt > MAX_NB_MTRR_VARIABLE) {
    panic("!#MTRR VCNT [%d]\n", mtrr_cap.vcnt);
  }
  uint64_t mask, len;
  nb_enabled_var_mtrr = 0;
  for (i = 0; i < mtrr_cap.vcnt; i++) {
    mtrr_variable[i].msr_base.address = MSR_ADDRESS_IA32_MTRR_PHYSBASE0 + 2*i;
    mtrr_variable[i].msr_mask.address = MSR_ADDRESS_IA32_MTRR_PHYSBASE0 + 2*i + 1;
    mtrr_variable[i].msr_base.value = msr_read(mtrr_variable[i].msr_base.address);
    mtrr_variable[i].msr_mask.value = msr_read(mtrr_variable[i].msr_mask.address);
    mtrr_variable[i].range_address_begin = ((uint64_t) mtrr_variable[i].msr_base.phybase) << 12;
    mask = ((uint64_t) mtrr_variable[i].msr_mask.phymask) << 12;
    len = (~mask) & last_addr;
    mtrr_variable[i].range_address_end = mtrr_variable[i].range_address_begin + len;

    // sort variable MTRRs by their begin address
    if (mtrr_variable[i].msr_mask.valid) {
      for (j = nb_enabled_var_mtrr; j > 0 && is_inf(&mtrr_variable[i], sorted_mtrr_variable[j-1]); j--) {
        sorted_mtrr_variable[j] = sorted_mtrr_variable[j-1];
      }
      sorted_mtrr_variable[j] = &mtrr_variable[i];
      nb_enabled_var_mtrr++;
    }
  }

  //
  // Create ranges
  //

  uint8_t k=0;                  // current position in the memory_range array
  uint64_t current_addr = 0;    // everything below this address has been processed
  uint8_t current_type;

  /* First fixed MTRRs */
  if (mtrr_def_type.fe) {   // First check if fixed MTRR are enabled
    memory_range[0].range_address_begin = 0;
    memory_range[0].range_address_end = -1;
    memory_range[0].type = mtrr_fixed[0].msr.value & 0xff;
    for (i = 0; i < 11; i++) {
      for (j = 0; j < 8; j++) {
        current_type = (mtrr_fixed[i].msr.value >> (j * 8)) & 0xff;
        if (memory_range[k].type == current_type) {
          memory_range[k].range_address_end += mtrr_fixed[i].sub_range_size;
        } else {
          k++;
          memory_range[k].range_address_begin = memory_range[k-1].range_address_end + 1;
          memory_range[k].range_address_end = memory_range[k].range_address_begin + mtrr_fixed[i].sub_range_size - 1;
          memory_range[k].type = current_type;
        }
      }
    }
    current_addr = 0x100000;
  } else {
    // init for var MTRRs
    memory_range[0].range_address_begin = 0;
    // We have to find out the type of the first range
    current_type = MEMORY_TYPE_DEFAULT;
    for (i=0; (i < nb_enabled_var_mtrr) && (sorted_mtrr_variable[i]->range_address_begin == 0); i++) {
      current_type=get_type(current_type, sorted_mtrr_variable[i]->msr_base.type);
    }
    memory_range[0].type = real_type(current_type);
  }

  /* Then var MTRRs */
  compute_var_MTRR(&k, &current_addr, 0, MEMORY_TYPE_DEFAULT, last_addr);

  /* We have to close the last range */
  fill_gap(&k, &current_addr, last_addr+1, MEMORY_TYPE_DEFAULT);
  memory_range[k].range_address_end = last_addr;
  nb_memory_range = k+1;

  /* At last we check memory ranges type just in case */
  for (i=0; i<nb_memory_range; i++) {
    check_memory_type(memory_range[i].type);
  }

  return 0;
}

/* Compute the variable MTRRs
 * Parameters:
 * - k            : current index of the memory_range array
 * - current_addr : address from which we need to compute the ranges (everything below this address have already been processed)
 * - begin_index  : begin index of the sorted_mtrr_variable array (for performance purpose, if we set this to 0 it will works too)
 * - type         : type of the range underneath (the range inside which we are)
 * - max_addr     : we need to stop processing this step when the range underneath is done
 */
static void compute_var_MTRR(uint8_t *k, uint64_t *current_addr, uint8_t begin_index, uint8_t type, uint64_t max_addr) {
  uint8_t i;                // index of the current mtrr_variable (current range)
  uint8_t current_type;     // result of the overlap of underneath range and current range
  uint64_t next_max_addr;   // last address of the overlap between current range and underneath range
  for (i=begin_index; i < nb_enabled_var_mtrr && sorted_mtrr_variable[i]->range_address_begin < max_addr; i++) {
    // everything below current_addr have already been processed
    if(sorted_mtrr_variable[i]->range_address_end < *current_addr) {
      continue;
    }

    // We fill the gap between current_addr and the beginning of the current zone
    fill_gap(k, current_addr, sorted_mtrr_variable[i]->range_address_begin, type);

    current_type = get_type(type, sorted_mtrr_variable[i]->msr_base.type);
    next_max_addr = min(sorted_mtrr_variable[i]->range_address_end, max_addr);
    // We look for overlap inside this overlap (overlapception)
    compute_var_MTRR(k, current_addr, i+1, current_type, next_max_addr);

    // We fill the gap between current_addr the last_addr to process
    fill_gap(k, current_addr, next_max_addr +1, current_type);
  }
}

/* Fill the gap between current_addr and to (exclusive) with the given memory type */
static void fill_gap(uint8_t *k, uint64_t *current_addr, uint64_t to, uint8_t type) {
  if (*current_addr < to) {
    if (memory_range[*k].type != real_type(type)) {
      // we close the last range if types are differents
      memory_range[*k].range_address_end = *current_addr - 1;
      // and then we create a new range
      (*k)++;
      memory_range[*k].range_address_begin = *current_addr;
      memory_range[*k].type = real_type(type);
    }
    *current_addr = to;
  }
}

inline uint8_t mtrr_get_nb_variable_mtrr(void) {
  return mtrr_cap.vcnt;
}

const struct memory_range *mtrr_get_memory_range(uint64_t address) {
  uint8_t i;
  for (i=0; i < nb_memory_range; i++) {
    if (address < memory_range[i].range_address_end) {
      return &memory_range[i];
    }
  }
  return NULL;
}

void mtrr_print_ranges(void) {
  uint8_t i;
  for (i=0; i < nb_memory_range; i++) {
    printk("0x%016X-0x%016X %d\n" , memory_range[i].range_address_begin
                                  , memory_range[i].range_address_end
                                  , memory_range[i].type);
  }
}
