#ifndef __ENV_H__
#define __ENV_H__

#include "vmm.h"
#include "stdint.h"

#define ENV_LIMIT 16

enum ENV_ERROR {
  ENV_OK,
  ENV_ERROR,
  ENV_NO_SPACE
};

typedef struct _env_command {
  uint8_t (*init) (void);
  uint8_t (*call) (struct registers *guest_regs);
  uint8_t (*execute) (void);
} env_command;

#define ENV_ID 0x1

uint8_t env_add_command(env_command *command);
void env_execute(void);
void env_init(void);
void env_call(struct registers *guest_regs);

#endif//__ENV_H__

