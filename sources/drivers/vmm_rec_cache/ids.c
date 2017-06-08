#include "stdio.h"
#include "hook.h"
#include "vmm.h"
#include "l3.h"

int timer(struct registers *gr) {
  l3();
  return 0;
}

void hook_main(void) {
  // Install some hooks
  hook_boot[EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED] = &timer;
  // L3 init
  l3_init();
}
