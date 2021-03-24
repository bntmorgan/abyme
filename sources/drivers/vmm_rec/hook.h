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
