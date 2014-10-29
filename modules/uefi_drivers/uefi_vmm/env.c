#include "env.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "efi/efi_eric.h"
#include "env/challenge.h"

#include <efi.h>
#include <efilib.h>

static env_command commands[ENV_LIMIT];
static uint32_t index = 0;
static uint8_t eric_present;
protocol_eric *eric;

uint8_t code[0x1000] __attribute__((aligned(0x1000)));

void (*challenge) (void) = (void *)&code[8];

challenge_system_table systab;

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
}

void env_init(void) {
  uint32_t i;
  if (eric_present) {
    for (i = 0; i < index; i++) {
      (*commands[i].init)();
    }
  }
}

void env_call(struct registers *guest_regs) {
  uint32_t i;
  if (eric_present) {
    for (i = 0; i < index; i++) {
      (*commands[i].call)(guest_regs);
    }
  }
}

void env_execute(void) {
  uint32_t i;
  if (eric_present) {
    // Challenge
    INFO("Copy challenge !\n");
    memcpy(&code[0], (uint8_t *)(uintptr_t)eric->rom, 0x1000);
    // Copy challenge system table
    *(uint64_t *)&code[0] = (uint64_t)&systab;
    // Execution
    INFO("Execute !\n");
    challenge();
    INFO("Write the solution !\n");
    // Write solution
    *(uint32_t *)(uintptr_t)eric->bar0 = 0xcafebabe;
    // Commands !
    for (i = 0; i < index; i++) {
      (*commands[i].execute)();
    }
  }
}
