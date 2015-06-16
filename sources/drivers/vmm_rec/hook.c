#include "hook.h"

void hook_main(void) {}

/**
 * Pre guest hv delegation callbacks
 */
vmexit_hook_boot hook_boot[HOOK_MAX_EXIT_REASON];

/**
 * Pre vmexit_handler callbacks
 */
vmexit_hook_pre hook_pre[HOOK_MAX_EXIT_REASON];

/**
 * Post vmexit_handler callbacks
 */
vmexit_hook_post hook_post[HOOK_MAX_EXIT_REASON];
