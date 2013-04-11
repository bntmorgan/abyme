#ifndef __VMEM_H__
#define __VMEM_H__

#include "types.h"

#include "vmm_info.h"

/*
 * The GDT is configured in flat mode by the loader and we use identity mapping
 * for pagination (without offset). As a consequence,
 * virtual address == physical address.
 * See [Intel_August_2012], volume 3, section 3.2.1.
 */
#define VMEM_ADDR_VIRTUAL_TO_PHYSICAL(addr)	((uint64_t) addr)
#define VMEM_ADDR_PHYSICAL_TO_VIRTUAL(addr)	((uint64_t) addr)

void vmem_print_info(void);
void vmem_get_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry);
void vmem_save_gdt(void);
void vmem_print_gdt(gdt_lm_ptr_t *gdt_ptr);

#define VMEM_GDT_SIZE 0x48
extern uint8_t vmem_gdt[VMEM_GDT_SIZE];
extern gdt_lm_ptr_t gdt_ptr, idt_ptr, saved_gdt_ptr;

#define VMEM_GET_SAVED_GDT_ENTRY(sel, desc) ( \
  (vmem_get_gdt_desc(vmem_gdt + (sel), (desc)))\
)

#define VMEM_PRINT_GDT_ENTRY(entry) { \
  INFO("  base=%08x limit=%08x access=%02x granularity=%02x\n", \
    (entry)->base, (entry)->limit, (entry)->access, (entry)->granularity); \
}

#endif
