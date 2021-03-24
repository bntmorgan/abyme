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

.global asm_probe

asm_probe:
  // Set
  mov %rax, %r9
  // Set size
  mov %rbx, %r10
  // Candidate
  mov %rcx, %r11
  mfence
  lfence
  // Load candidate cache line
  mov (%r11), %rax
  // for (i = 0; i < ss; i++)
  // i = 0
  xor %r12, %r12
  // Last line =0
  xor %r15, %r15
loop:
  cmp %r12, %r10
  jz endloop
  // %rax = set[i]
  mov (%r9, %r12, 8), %rax
  mfence
  lfence
  lea (%r9, %r12, 8), %r15
  mov (%rax), %rax
  // i++
  inc %r12
  jmp loop
endloop:
  // %r12 = rdtscp
  mfence
  lfence
  rdtscp
  mov %rax, %r12
  shl $32, %rdx
  or %rdx, %r12
  // Access candidate cache line
  mov (%r11), %rax
  // %r13 = rdtscp
  rdtscp
  mov %rax, %r13
  shl $32, %rdx
  or %rdx, %r13
  sub %r12, %r13
  mov %r13, %rax
  mov %r15, %rcx
  retq
