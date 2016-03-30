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
