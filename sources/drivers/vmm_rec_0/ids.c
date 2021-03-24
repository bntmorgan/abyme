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

const char *yolo = "SSTIC-AWESOME-ROCKS-BABE";

int cpuid_pre(struct registers *gr) {
  if (gr->rax == 0xaaaaaaaa) {
    INFO("Modifying rbx\n");
    gr->rbx = (uint64_t)yolo[7] << 56 | (uint64_t)yolo[6] << 48 |
      (uint64_t)yolo[5] << 40 | (uint64_t)yolo[4] << 32 |
      (uint64_t)yolo[3] << 24 | (uint64_t)yolo[2] << 16 |
      (uint64_t)yolo[1] << 8 | (uint64_t)yolo[0] << 0;
  }
  return 0;
}

void hook_main(void) {
  // Install some hooks
  hook_boot[EXIT_REASON_CPUID] = &cpuid_pre;
}
