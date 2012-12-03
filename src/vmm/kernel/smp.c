/*
 * TODO: intel 8.4.4
 */

#define SMP_ADDRESS_EBDA_ADDRESS	0x0040e
#define SMP_ADDRESS_BASE_MEMORY_SIZE	0x00413
#define SMP_ADDRESS_ROM_START		0xf0000
#define SMP_ADDRESS_ROM_END		0xfffff

#define SMP_APIC_ICR_LOW	0xfee00300
#define SMP_APIC_ICR_HIGH	0xfee00310
#define SMP_APIC_SVR		0xfee000f0
#define SMP_APIC_APIC_ID	0xfee00020
#define SMP_APIC_LVT3		0xfee00370
#define SMP_APIC_APIC_ENABLED	0x100

#define SMP_ICR_DELIVERY_STATUS (1 << 11)
#define SMP_ICR_DELIVERY_STATUS_SEND_PENDING (1 << 11)

#define SMP_AP_VECTOR 0x1

#include <stdint.h>

#include "common/string_int.h"
#include "arch/msr_int.h"
#include "vmem_int.h"

#define SMP_MP_PROCESSOR_ENTRY_TYPE		0
#define SMP_MP_BUS_ENTRY_TYPE			1
#define SMP_MP_IO_APIC_ENTRY_TYPE		2
#define SMP_MP_IO_INTERRUPT_ENTRY_TYPE		3
#define SMP_MP_LOCAL_INTERRUPT_ENTRY_TYPE	4

#define SMP_MP_CPU_FLAGS_EN	0x1
#define SMP_MP_CPU_FLAGS_BP	0x2

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.1:
 */
typedef struct {
  uint32_t signature;
  uint32_t physical_address_pointer;
  uint8_t length;
  uint8_t spec_rev;
  uint8_t checksum;
  uint8_t features1;
  uint8_t features2;
} __attribute__((packed)) smp_mp_floating_pointer_structure_t;

typedef struct {
  uint32_t signature;
  uint16_t base_table_length;
  uint8_t spec_rev;
  uint8_t checksum;
  uint8_t oemid[8];
  uint8_t prodid[12];
  uint32_t oem_table_pointer;
  uint16_t oem_table_size;
  uint16_t entry_count;
  uint32_t memory_mapped_address_of_local_apic;
  uint16_t extended_table_length;
  uint8_t extended_table_checksum;
  uint8_t reserved;
} __attribute__((packed)) smp_mp_configuration_table_header_t;

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.3.1:
 */
typedef struct {
  uint8_t entry_type;
  uint8_t local_apic_id;
  uint8_t local_apic_version;
  uint8_t cpu_flags;
  uint32_t cpu_signature;
  uint32_t feature_flags;
  uint32_t reserved_1;
  uint32_t reserved_2;
} __attribute__((packed)) smp_mp_processor_entry_t;

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.3.2:
 */
typedef struct {
  uint8_t entry_type;
  uint8_t bus_id;
  uint8_t bus_type_string[6];
} __attribute__((packed)) smp_mp_bus_entry_t;

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.3.3:
 */
typedef struct {
  uint8_t entry_type;
  uint8_t io_apic_id;
  uint8_t io_apic_version;
  uint8_t io_apic_flags;
  uint32_t memory_mapped_address_of_io_apic;
} __attribute__((packed)) smp_mp_io_apic_entry_t;

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.3.4:
 */
typedef struct {
  uint8_t entry_type;
  uint8_t interrupt_type;
  uint16_t io_interrupt_flag;
  uint8_t source_bus_id;
  uint8_t source_bus_irq;
  uint8_t destination_io_apic_id;
  uint8_t destination_io_apic_intin;
} __attribute__((packed)) smp_mp_io_interrupt_entry_t;

/*
 * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.3.5:
 */
typedef struct {
  uint8_t entry_type;
  uint8_t interrupt_type;
  uint16_t local_interrupt_flag;
  uint8_t source_bus_id;
  uint8_t source_bus_irq;
  uint8_t destination_local_apic_id;
  uint8_t destination_local_apic_lintin;
} __attribute__((packed)) smp_mp_local_interrupt_entry_t;

uint8_t smp_nb_cpus;
uint8_t smp_bsp_id;
smp_mp_floating_pointer_structure_t *smp_mp_floating_pointer_structure;
smp_mp_configuration_table_header_t *smp_mp_configuration_table_header;

uint8_t smp_cpu_ids[16];
uint8_t smp_nb_cpus;
uint8_t smp_bsp_id;

extern uint8_t trampoline_start;
extern uint8_t trampoline_end;

uint64_t smp_search_mp_floating_pointer_structure(uint64_t address, uint64_t length) {
  uint8_t mp_fps_signature[] = "_MP_";
  for (uint32_t i = 0; i < length - sizeof(mp_fps_signature); i++) {
    if ( *((uint32_t *) (address + i)) == *((uint32_t *) &mp_fps_signature[0])) {
      return address + i;
    }
  }
  return 0;
}

void smp_search_mp_configuration_table_header(void) {
  /*
   * See Intel MultiProcessor Specification, Version 1.4, May 1997, section 4.
   */
  uint64_t area_start = ((uint64_t) *((uint32_t *) SMP_ADDRESS_EBDA_ADDRESS));
  INFO("search smp_mp_floating_pointer_structure into EBDA (%x obtained from %x)\n",
      area_start, SMP_ADDRESS_EBDA_ADDRESS);
  uint64_t ret = smp_search_mp_floating_pointer_structure(area_start, 1024);
  if (ret == 0) {
    area_start = ((uint64_t) *((uint32_t *) SMP_ADDRESS_BASE_MEMORY_SIZE)) << 10;
    INFO("search smp_mp_floating_pointer_structure into base memory (%x obtained from %x)\n", area_start,
        SMP_ADDRESS_BASE_MEMORY_SIZE);
    ret = smp_search_mp_floating_pointer_structure(area_start - 1024, 1024);
    if (ret == 0) {
      INFO("search smp_mp_floating_pointer_structure into rom (%x)\n", SMP_ADDRESS_ROM_START);
      ret = smp_search_mp_floating_pointer_structure(SMP_ADDRESS_ROM_START,
          SMP_ADDRESS_ROM_END - SMP_ADDRESS_ROM_START);
    }
  }
  smp_mp_floating_pointer_structure = (smp_mp_floating_pointer_structure_t *) ret;
  if (ret != 0 && smp_mp_floating_pointer_structure->physical_address_pointer != 0) {
    INFO("found mp floating pointer structure at %08x%08x\n", (uint32_t) (ret >> 32), (uint32_t) ret);
    smp_mp_configuration_table_header = (smp_mp_configuration_table_header_t *)
        ((uint64_t) smp_mp_floating_pointer_structure->physical_address_pointer);
  }
}

void smp_print_mp_configuration_table_header(void) {
  uint8_t oemid[5];
  oemid[4] = 0;
  for (uint8_t i = 0; i < 4; i++) {
    oemid[i] = smp_mp_configuration_table_header->oemid[i];
  }
  uint8_t prodid[13];
  prodid[12] = 0;
  for (uint8_t i = 0; i < 12; i++) {
    prodid[i] = smp_mp_configuration_table_header->prodid[i];
  }
  INFO("smp_mp_floating_pointer_structure: oemid='%s' prodid='%s'\n", oemid, prodid);
  INFO("  entry count='%d'\n", smp_mp_configuration_table_header->entry_count);
}

void smp_process_processor_entry(smp_mp_processor_entry_t *smp_mp_processor_entry) {
  if ((smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_EN) == SMP_MP_CPU_FLAGS_EN) {
    INFO("processor local_apic_id=%x\n", (uint32_t) smp_mp_processor_entry->local_apic_id);
    INFO("          local_apic_version=%x\n", (uint32_t) smp_mp_processor_entry->local_apic_version);
    INFO("          cpu_flags=%x\n", (uint32_t) smp_mp_processor_entry->cpu_flags);
    INFO("          cpu_signature=%x\n", smp_mp_processor_entry->cpu_signature);
    INFO("          feature_flags=%x\n", smp_mp_processor_entry->feature_flags);
    INFO("          is bsp=%d\n", (uint32_t) (smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_BP));
    /*
     * Note: "Local APIC IDs must be unique, and need not be consecutive",
     * as described in section 3.6.6 of 
     * Intel MultiProcessor Specification, Version 1.4, May 1997.
     */
    smp_cpu_ids[smp_nb_cpus] = smp_mp_processor_entry->local_apic_id;
    smp_nb_cpus = smp_nb_cpus + 1;
    if ((smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_BP) == SMP_MP_CPU_FLAGS_BP) {
      smp_bsp_id = smp_nb_cpus - 1;
    }
  }
}

void smp_process_entries(void) {
  uint8_t *entry_ptr = ((uint8_t *) smp_mp_configuration_table_header) +
      sizeof(smp_mp_configuration_table_header_t);
  for (uint32_t i = 0; i < smp_mp_configuration_table_header->entry_count; i++) {
    uint8_t entry_type = *entry_ptr;
    if (entry_type == SMP_MP_PROCESSOR_ENTRY_TYPE) {
      smp_process_processor_entry((smp_mp_processor_entry_t *) entry_ptr);
      entry_ptr = entry_ptr + sizeof(smp_mp_processor_entry_t);
    } else if (entry_type == SMP_MP_BUS_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_bus_entry_t);
    } else if (entry_type == SMP_MP_IO_APIC_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_io_apic_entry_t);
    } else if (entry_type == SMP_MP_IO_INTERRUPT_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_io_interrupt_entry_t);
    } else if (entry_type == SMP_MP_LOCAL_INTERRUPT_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_local_interrupt_entry_t);
    } else {
      ERROR("invalid entry type");
    }
  }
}

void smp_default_setup(void) {
  smp_nb_cpus = 1;
  smp_mp_configuration_table_header = 0;
  smp_mp_floating_pointer_structure = 0;
  smp_nb_cpus = 0;
  smp_bsp_id = 255;
}

void smp_activate_ap(void) {
  /*
   * From Intel section 8.4.4.1.
   */
  /*
   * Point 11: "Enable the local APIC by setting bit 8 of the APIC spurious
   * vector register (SVR)".
   * TODO: 0x20 is the "interruption". We should activate IDT!
   */
  uint32_t *svr_address = (uint32_t *) SMP_APIC_SVR;
  *svr_address = 0x20 | SMP_APIC_APIC_ENABLED;
  /*
   * Point 12.
   */
  uint32_t *lvt3_address = (uint32_t *) SMP_APIC_LVT3;
  *lvt3_address = ((*lvt3_address) & 0xffffff00) | 0x3;
  /*
   * Point 13: TODO.
   */
  /*
   * Point 14-18.
   */
  uint32_t *icr_address_low = (uint32_t *) SMP_APIC_ICR_LOW;
  uint32_t *icr_address_high = (uint32_t *) SMP_APIC_ICR_HIGH;
  for (uint8_t i = 0; i < smp_nb_cpus; i++) {
    if (i != smp_bsp_id) {
      INFO("activating cpu %d having local apic id %x\n", i, smp_cpu_ids[i]);
      /*
       * See figure 10.12.
       */
      *icr_address_high = ((uint32_t) smp_cpu_ids[i]) << 24;
      *icr_address_low = 0x00004500;
      while ((*icr_address_low & SMP_ICR_DELIVERY_STATUS) == SMP_ICR_DELIVERY_STATUS_SEND_PENDING);
      *icr_address_high = ((uint32_t) smp_cpu_ids[i]) << 24;
      *icr_address_low = 0x00004600 | SMP_AP_VECTOR;
      while ((*icr_address_low & SMP_ICR_DELIVERY_STATUS) == SMP_ICR_DELIVERY_STATUS_SEND_PENDING);
      /*
       * TODO!
       */
      INFO("launch only one core!\n");
      break;
    }
  }
}

void smp_print_info(void) {
  uint32_t eax;
  uint32_t edx;
  msr_read(MSR_IA32_APIC_BASE, &eax, &edx);
  INFO("apic physical address: %08x%08x\n", edx, eax & 0xfffff000);
}

void smp_activate_apic(void) {
  uint32_t eax;
  uint32_t edx;
  msr_read(MSR_IA32_APIC_BASE, &eax, &edx);
  msr_write(MSR_IA32_APIC_BASE, eax | 0x800, edx);
}

void smp_ap_fcn(void) {
  uint64_t p = 0;
  for (uint16_t k = 0; k < 0x40; k++)
    for (uint16_t i = 0; i < 0xffff; i++)
      p += i;
  printk("I am an AP! %08x%08x\n", (uint32_t) (p >> 32), (uint32_t) p);
  uint32_t counter_x = 0;
  uint32_t counter_y = 0;
  uint32_t nb_columns = 80;
  uint16_t *pos = (uint16_t *) ((counter_x + counter_y * nb_columns) * sizeof(uint16_t) + 0xb8001);
  uint16_t number = 0;
  while (1) {
    /*
     * A 16 bits number contains 5 digits in decimal.
     */
    for (uint8_t j = 0; j < 5; j++) {
      uint16_t tmp = number;
      for (uint8_t k = 0; k < j; k++) {
        tmp = tmp / 10;
      }
      pos[5 - j] = (uint16_t) (((uint16_t) '0' + (tmp % 10))) | 0x3d00;
    }
    for (uint16_t k = 0; k < 0xf; k++)
      for (uint16_t i = 0; i < 0xffff; i++);
    number = number + 1;
  }
  while (1);
}

void smp_prepare_trampoline(void) {
  uint8_t *ptr_start = &trampoline_start;
  uint8_t *ptr_end = &trampoline_end;
  uint8_t *ptr_dst = (uint8_t *) (SMP_AP_VECTOR << 12);
  while (ptr_start < ptr_end) {
    *ptr_dst = *ptr_start;
    ptr_start++;
    ptr_dst++;
  }
  /*
   * TODO: add a signature into the trampoline to identify the header!
   */
  uint64_t *trampoline_cr3 = (uint64_t *) ((uint8_t *) ((SMP_AP_VECTOR << 12) + 12));
  uint8_t *trampoline_gdt = ((uint8_t *) ((SMP_AP_VECTOR << 12) + 4));
  uint64_t *trampoline_fcn = ((uint8_t *) ((SMP_AP_VECTOR << 12) + 20));
  cpu_read_gdt(trampoline_gdt);
  cpu_read_cr3(trampoline_cr3);
  *trampoline_fcn = (uint64_t) smp_ap_fcn;
  INFO("trampoline cr3: %08x%08x\n", (uint32_t) (*trampoline_cr3 >> 32), (uint32_t) *trampoline_cr3);
}

void smp_print_trampoline(uint8_t *area) {
  uint8_t *ptr_start = area;
  uint8_t *ptr_end = (uint8_t *) (((uint64_t) &trampoline_end) - ((uint64_t) &trampoline_start) + area);
  uint8_t line_size = 16;
  uint8_t previous_line_hidden = 0;
  while (ptr_start < ptr_end) {
    uint8_t must_print_line = 0;
    for (uint8_t j = 0; j < line_size && (&ptr_start[j] < ptr_end); j++) {
      if (ptr_start[j] != 0x0) {
        must_print_line = 1;
        break;
      }
    }
    if (must_print_line == 1) {
      if (previous_line_hidden == 1) {
        printk("...\n");
      }
      previous_line_hidden = 0;
      for (uint8_t j = 0; j < line_size && (&ptr_start[j] < ptr_end); j++) {
        printk("%02x ", ptr_start[j]);
      }
      printk("\n");
    } else {
      previous_line_hidden = 1;
    }
    ptr_start = &ptr_start[line_size];
  }
  printk("\n");
}

void smp_setup(void) {
  /*
   * TODO: check the APIC physical address equals the virtual address!
   */
  smp_print_info();
  smp_default_setup();
  smp_activate_apic();
  smp_print_trampoline((uint8_t *) &trampoline_start);
  smp_prepare_trampoline();
  smp_print_trampoline((uint8_t *) (SMP_AP_VECTOR << 12));
  smp_search_mp_configuration_table_header();
  if (smp_mp_configuration_table_header != 0) {
    smp_print_mp_configuration_table_header();
    smp_process_entries();
    smp_activate_ap();
  }
}
