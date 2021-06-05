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

#include <stdint.h>

#include "stdio.h"
#include "cpuid.h"
#include "paging.h"
#include "cpu.h"

struct kernel_state {
  uint64_t cr3;
  uint64_t rbp;
  uint64_t rsp;
};

int initialized = 0;

int cr3_saved = 0;

struct kernel_state state;

// Page tables
struct paging_ia32e_1gb __attribute__((section(".bss.zizi"))) pages;

// Initialize kernel
void kernel_init(void) {
  // Set how to putc
#ifdef _QEMU
  putc = &qemu_putc;
#else
  putc = &no_putc;
#endif
  // Setup cpuid
  cpuid_setup();
  // Setup paging
  paging_setup_host_paging_1gb(&pages);
  INFO("INIT done mamene !\n");
}

// Set kernel configuration as memory for instance
void kernel_set(void) {
  // Save caller state
  state.cr3 = cpu_read_cr3();

  if (state.cr3 == (uint64_t)&pages.PML4[0]) {
    cr3_saved = 0;
  } else {
    // Set our memory state
    cpu_write_cr3((uint64_t)&pages.PML4[0]);
    cr3_saved = 1;
  }
}

// Restore caller's configuration as memory for instance
void kernel_restore(void) {
  // Restore caller state if any
  if (cr3_saved) {
    cpu_write_cr3(state.cr3);
  }
}

// Program entry point. Specific .start section is used to force position in the
// beginning of .text section. See linker.ld
void __attribute__((section(".start"))) kernel_start(void) {

  // First call by the loader
  if (initialized == 0) {
    initialized = 1;
    kernel_init();
    return;
  }

  // Elsewise we are in SMM mode, we can play !
  INFO("Party harder\n");
}
