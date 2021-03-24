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

#include "stdio.h"
#include "hook.h"
#include "vmm.h"
#include "env.h"

int vmcall_boot(struct registers *gr) {
  env_call(gr);
  return 1;
}

int preemption_timer_expired_boot(struct registers *gr) {
  env_execute();
  return 0;
}

void hook_main(void) {
  // Install some hooks
  hook_boot[EXIT_REASON_VMCALL] = &vmcall_boot;
  hook_boot[EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED] =
    &preemption_timer_expired_boot;
  hook_pre[EXIT_REASON_IO_INSTRUCTION] = &env_io_instruction;

  // Env setup
  env_setup();
  env_init();
}
