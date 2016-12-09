#include "env.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "efi/efi_eric.h"
#include "env/challenge.h"
#include "env_md5.h"
#include "env_flash.h"
#include "vmx.h"
#include "cpu.h"
#include "msr.h"
#include "vmcs.h"
#include "paging.h"
#include "efiw.h"
#include "debug.h"
#include "ept.h"
#include "pci.h"
// XXX
#include "apic.h"

#include <efi.h>
#include <efilib.h>

static env_command commands[ENV_LIMIT];
static uint32_t index = 0;
static uint8_t eric_present;
static uint8_t env_enabled;;
static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;
protocol_eric *eric;

uint8_t *trap_pci;

uint8_t *code;

void (*challenge) (void);

challenge_system_table systab;

uint64_t env_tsc_to_micro(uint64_t t) {
  return t / tsc_freq_MHz;
}

uint8_t env_add_command(env_command *command) {
  if (index < ENV_LIMIT) {
    commands[index] = *command;
    index++;
    return ENV_OK;
  } else {
    return ENV_NO_SPACE;
  }
}

// XXX unduplicate (sources/drivers/vmm_rec/pci.c)
static uint8_t protect;
uint8_t env_no_protect_out(uint16_t port, uint32_t value) {
  uint32_t pci_addr = pci_make_addr(PCI_MAKE_ID(eric->pci_addr.bus,
        eric->pci_addr.device, eric->pci_addr.function));
  if (port == PCI_CONFIG_ADDR) {
    if ((value & ~(0xff)) == pci_addr) {
      protect = 1;
    } else {
      protect = 0;
    }
    return 1;
  } else if (port == PCI_CONFIG_DATA) {
    return 1 - protect;
  }
  return 1;
}

uint8_t env_no_protect_in(uint16_t port) {
  if (port == PCI_CONFIG_ADDR) {
    return 1;
  } else if (port == PCI_CONFIG_DATA) {
    return 1 - protect;
  }
  return 1;
}

// XXX unduplicate (sources/drivers/vmm_rec/pci.c)
static uint64_t get_mmio_addr(void) {
  uint64_t MMCONFIG_base;
  uint8_t MMCONFIG_length;
  uint16_t MMCONFIG_mask;
  uint64_t base_addr;
  uint64_t MMCONFIG;

  /* MMCONFIG_length : bit 1:2 of PCIEXBAR (offset 60) */
  MMCONFIG_length = (pci_readb(0,0x60) & 0x6) >> 1;

  /* MMCONFIG corresponds to bits 38 to 28 of the pci base address
     MMCONFIG_length decrease to 27 or 26 the lsb of MMCONFIG */
  switch (MMCONFIG_length) {
    case 0:
      MMCONFIG_mask = 0xF07F;
      break;
    case 1:
      MMCONFIG_mask = 0xF87F;
      break;
    case 2:
      MMCONFIG_mask = 0xFC7F;
      break;
    default:
      panic("Bad MMCONFIG Length\n");
  }

  /* MMCONFIG : bit 38:28-26 of PCIEXBAR (offset 60) -> 14:4-2 of PCIEXBAR + 3 */
  MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;
  MMCONFIG_base = ((uint64_t)MMCONFIG << 16);

  INFO("MMCONFIG : base(@0x%016X)\n", MMCONFIG_base);
  base_addr = MMCONFIG_base + PCI_MAKE_MMCONFIG(eth->pci_addr.bus,
      eth->pci_addr.device, eth->pci_addr.function);

  INFO("MMIO ERIC config space(@0x%016X)\n", base_addr);
  return base_addr;
}

void env_setup(void) {
  EFI_GUID guid_eric = EFI_PROTOCOL_ERIC_GUID;
  EFI_STATUS status = LibLocateProtocol(&guid_eric, (void **)&eric);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }
  INFO("ENV INIT : ERIC BAR0 %X\n", eric->bar0);
  systab.printk = &printk;
  systab.answer = (uint32_t *)(uintptr_t)eric->bar0;
  eric_present = 1;
  env_enabled = 1;

  // Allode code table
  code = efi_allocate_pages(1);
  challenge = (void *)&code[8];

  // Get the TSC frequency
#ifdef _NO_MSR_PLATFORM_INFO
  // XXX unsupported in qemu 2.7.0
#ifdef _CPU_FREQ_MHZ
  tsc_freq_MHz = _CPU_FREQ_MHZ;
#else
  tsc_freq_MHz = 5000;
#endif
#else
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
#endif
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;

  // Setup DSN experiment
//   env_command md5 = {
//     &env_md5_init,
//     &env_md5_call,
//     &env_md5_execute,
//     "md5"
//   };
//   env_add_command(&md5);
// 
//   env_command flash = {
//     &env_flash_init,
//     &env_flash_call,
//     &env_flash_execute,
//     "flash"
//   };
//   env_add_command(&flash);

  // Protect ERIC with EPT
  int m;
  uint64_t base_addr = get_mmio_addr();
  trap_pci = efi_allocate_pages(1);
  memset(trap_pci, 0xff, 0x1000);
  for (m = 0; m < VM_NB; m++) {
    ept_remap((uint64_t)eric->rom, (uint64_t)trap_pci, 0x7, m);
    ept_remap((uint64_t)eric->bar0, (uint64_t)trap_pci, 0x7, m);
    ept_remap(base_addr, (uint64_t)trap_pci, 0x7, m);
  }

  // Protect ERIC with PIO
}

void env_init(void) {
  uint32_t i;
  if (eric_present) {
    for (i = 0; i < index; i++) {
      (*commands[i].init)();
    }
  }
}

/**
 * Returns the string id of a command
 *
 * direction : c type : x86 long mode reg[ : comment]
 *
 * in : uint64_t command_id : rcx
 * out : uint64_t present : rdx
 * in out : char *user_buffer : rsi : 256 char table in userland
 */
void env_list(struct registers *guest_regs) {
  // Present
  if (guest_regs->rcx < index) {
    uint64_t paddr, cr3 = cpu_vmread(GUEST_CR3), *entry;
    uint8_t type;
    guest_regs->rdx = 0x1;
    // Walk VM addr
    if (paging_walk(cr3, guest_regs->rsi, &entry, &paddr, &type)) {
      INFO("ERROR walking guest address...\n");
      return;
    }
    // String copy
    memcpy((void *)paddr, (void *)commands[guest_regs->rcx].name,
        strlen((char *)commands[guest_regs->rcx].name) + 1);
  // Not present
  } else {
    guest_regs->rcx = 0x0;
  }
}

/**
 * VMCALL env handler
 */
void env_call(struct registers *guest_regs) {
  if (eric_present) {
    // List commands 
    if (guest_regs->rbx == ENV_VMCALL_LIST) {
      env_list(guest_regs);
    // Is rbx a registered command ?
    } else if (guest_regs->rbx == ENV_VMCALL_CALL) {
      (*commands[guest_regs->rcx].call)(guest_regs);
    } else if (guest_regs->rbx == ENV_VMCALL_DISABLE) {
      env_enabled = 0;
      INFO("CAUTION : env has been disabled\n");
    } else if (guest_regs->rbx == ENV_VMCALL_ENABLE) {
      env_enabled = 1;
      INFO("CAUTION : env is now enabled\n");
    } else if (guest_regs->rbx == ENV_VMCALL_ERIC_BAR0_WRITE) {
      uint32_t index = guest_regs->rcx >> 2;
      eric->bar0[index] = guest_regs->rdx & 0xffffffff;
    }
  }
}

/**
 * IO_INSTRUCTION env handler
 */
int env_io_instruction(struct registers *guest_regs) {
  VMR(info.qualification);
  uint64_t exit_qualification = vmcs->info.qualification.raw;
  uint8_t direction = exit_qualification & 8;
  uint8_t size = exit_qualification & 7;
  uint8_t string = exit_qualification & (1<<4);
  uint8_t rep = exit_qualification & (1<<5);
  uint16_t port = (exit_qualification >> 16) & 0xffff;

  // REP prefixed || String I/O Unsupported
  if (rep || string) {
    ERROR("I/O instruction with rep prefix unsupported\n");
  }

  // out
  uint32_t v = guest_regs->rax;
  if (direction == 0) {
    if (!env_no_protect_out(port, v)) {
      return 1;
    }
    // in
  } else {
    if (!env_no_protect_in(port)) {
      INFO("ERIC I/O config space block\n");
      if (size == 0) {
        guest_regs->rax = guest_regs->rax | 0x000000ff;
      } else if (size == 1) {
        guest_regs->rax = guest_regs->rax | 0x0000ffff;
      } else if (size == 3) {
        guest_regs->rax = guest_regs->rax | 0xffffffff;
      } else {
        ERROR("I/O size decoding error\n");
      }
      return 1;
    }
  }
  return 0;
}

void env_execute(void) {
  uint32_t i;
  uint64_t micros, a, b;
  if (eric_present && env_enabled) {
    // Challenge
    memcpy(&code[0], (uint8_t *)(uintptr_t)eric->rom, 0x1000);
    // Copy challenge system table
    *(uint64_t *)&code[0] = (uint64_t)&systab;
    // Execution
    a = cpu_rdtsc();
    challenge();
    b = cpu_rdtsc();
    // Compute time in microseconds
    micros = env_tsc_to_micro(b - a);
    INFO("Challenge execution time : 0x%016X microseconds\n", micros);
    // Commands !
    for (i = 0; i < index; i++) {
      (*commands[i].execute)();
    }
  }
}
