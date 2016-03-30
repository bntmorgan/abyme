#ifndef __HOOK_H__
#define __HOOK_H__

#include "vmm.h"

#define HOOK_MAX_EXIT_REASON 65

enum hook_override_modes {
  HOOK_OVERRIDE_NONE,
  HOOK_OVERRIDE_SKIP, // Skip instruction
  HOOK_OVERRIDE_STAY // Stay
};

void hook_main(void) __attribute__((weak));

typedef int (*vmexit_hook_boot)(struct registers *);
typedef int (*vmexit_hook_pre)(struct registers *);
typedef void (*vmexit_hook_post)(struct registers *);

extern vmexit_hook_boot hook_boot[HOOK_MAX_EXIT_REASON];
extern vmexit_hook_pre hook_pre[HOOK_MAX_EXIT_REASON];
extern vmexit_hook_post hook_post[HOOK_MAX_EXIT_REASON];

#endif//__HOOK_H__
