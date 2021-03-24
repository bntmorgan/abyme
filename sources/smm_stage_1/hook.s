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

.global hook
hook:

  // Save GPRs

  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rbp
  push %rbx
  push %rdx
  push %rcx
  push %rax

  // Save cr3

  mov %cr3, %rax
  push %rax

  // Set smm_stage_2 overall id mapping, see sbss symbol in smm_stage_2.elf
  // Page tables symbol is "pages".
  // It has to be id mapping to fit with efi current memory configuration
  movabs 0x100005000, %rax
  mov %rax, %cr3

  sub $0x8, %rsp
  // See kernel_start symbol in smm_stage_2.elf
  // Pushq $0x100000000
  movq $0x100000000, %rax
  pushq %rax
  // Call hook
  call *(%rsp)
  add $0x8, %rsp

  // Restore cr3

  pop %rax
  mov %rax, %cr3

  // Restore GPRs

  pop %rax
  pop %rcx
  pop %rdx
  pop %rbx
  pop %rbp
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15

  rsm
