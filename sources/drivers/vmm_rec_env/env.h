/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ENV_H__
#define __ENV_H__

#include "vmm.h"
#include "stdint.h"
#include "efi/efi_eric.h"

#define ENV_LIMIT 16

enum ENV_ERROR {
  ENV_OK,
  ENV_ERROR,
  ENV_NO_SPACE
};

enum ENV_VMCALL_IDS {
  ENV_VMCALL_LIST,
  ENV_VMCALL_CALL,
  ENV_VMCALL_ENABLE,
  ENV_VMCALL_DISABLE,
  ENV_VMCALL_ERIC_BAR0_WRITE
};

typedef struct _env_command {
  int (*init) (void);
  int (*call) (struct registers *guest_regs);
  int (*execute) (void);
  const char *name;
} env_command;

uint8_t env_add_command(env_command *command);
void env_execute(void);
void env_setup(void);
void env_init(void);
void env_call(struct registers *guest_regs);
int env_io_instruction(struct registers *guest_regs);

extern protocol_eric *eric;

#endif//__ENV_H__

