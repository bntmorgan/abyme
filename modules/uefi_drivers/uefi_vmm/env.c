#include "env.h"
#include "stdint.h"
#include "stdio.h"

static env_command commands[ENV_LIMIT];
static uint32_t index = 0;

uint8_t env_add_command(env_command *command) {
  if (index < ENV_LIMIT) {
    commands[index] = *command;
    index++;
    return ENV_OK;
  } else {
    return ENV_NO_SPACE;
  }
}

void env_init(void) {
  uint32_t i;
  for (i = 0; i < index; i++) {
    (*commands[i].init)();
  }
}

void env_call(struct registers *guest_regs) {
  uint32_t i;
  for (i = 0; i < index; i++) {
    (*commands[i].call)(guest_regs);
  }
}

void env_execute(void) {
  uint32_t i;
  for (i = 0; i < index; i++) {
    (*commands[i].execute)();
  }
}
