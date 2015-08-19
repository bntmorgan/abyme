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
#include "walk.h"

#include <efi.h>
#include <efilib.h>

static env_command commands[ENV_LIMIT];
static uint32_t index = 0;
static uint8_t eric_present;
static uint8_t env_enabled;;
static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;
protocol_eric *eric;

uint8_t code[0x1000] __attribute__((aligned(0x1000)));

void (*challenge) (void) = (void *)&code[8];

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

void env_setup(void) {
  EFI_GUID guid_eric = EFI_PROTOCOL_ERIC_GUID;
  EFI_STATUS status = LibLocateProtocol(&guid_eric, (void **)&eric);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }
  printk("ENV INIT : ERIC BAR0 %X\n", eric->bar0);
  systab.printk = &printk;
  systab.answer = (uint32_t *)(uintptr_t)eric->bar0;
  eric_present = 1;
  env_enabled = 1;

  // TSC configuration
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;

  // Setup DSN experiment
  env_command md5 = {
    &env_md5_init,
    &env_md5_call,
    &env_md5_execute,
    "md5"
  };
  env_add_command(&md5);

  env_command flash = {
    &env_flash_init,
    &env_flash_call,
    &env_flash_execute,
    "flash"
  };
  env_add_command(&flash);
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
    uint64_t paddr, cr3 = cpu_vmread(GUEST_CR3), size;
    guest_regs->rdx = 0x1;
    // Walk VM addr
    if (walk_long(cr3, guest_regs->rsi, &paddr, &size)) {
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
      INFO("CATION : env has been disabled\n");
    } else if (guest_regs->rbx == ENV_VMCALL_ENABLE) {
      env_enabled = 1;
      INFO("CATION : env is now enabled\n");
    } else if (guest_regs->rbx == ENV_VMCALL_ERIC_BAR0_WRITE) {
      uint32_t index = guest_regs->rcx >> 2;
      eric->bar0[index] = guest_regs->rdx & 0xffffffff;
    }
  }
}

void env_execute(void) {
  uint32_t i;
  uint64_t micros, a, b;
  if (eric_present && env_enabled) {
    // Challenge
    INFO("Copy and execute challenge !\n");
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
