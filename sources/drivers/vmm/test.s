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

trampoline_start:
.global trampoline_start

.global start16
.org 0

.code16
start16:
  jmp start16_cont
.org 4

GDT: .long 0
     .long 0
CR3: .long 0
     .long 0
FCN: .long 0
     .long 0

start16_cont:
  /*
   * Set the stack. The vmm will change the $0x50 in this code with a different
   * value in order to be thread safe for multi-core (the stacks must not
   * overlap).
   */
  mov $0x50, %ax
  mov %ax, %ss
  mov $0xfe, %sp
  /*
   * Change cs:ip in order to have ip = 0 for the first byte of this code.
   * If this code is loaded at the physical address 0x1000, the address of
   * the first byte can be 0x100:0x0000 or 0x000:0x10000. We change into
   * 0x100:0x0000 because the compiler set the addresses relative to the first
   * byte of this binary (org directive).
   */
  xor %edx, %edx
  xor %ebx, %ebx
  xor %ax, %ax
  call get_address
get_address:
  test %ax, %ax
  jnz cs_adjusted 
  pop %bx
  /* ax = get_address */
  mov %bx, %ax
  /* edx == (cs << 4) */
  mov %cs, %dx
  shl $0x4, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] */
  add %ebx, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] - $get_address */
  sub %eax, %edx
  /* edx == (new_cs << 4) */
  /* ecx == new_cs */
  mov %edx, %ecx
  shr $0x4, %ecx
  /*
   * Simulate a long jump to get_address using long ret.
   */
  pushw %cx
  pushw %ax
  lret
cs_adjusted:
  /*
   * Update data segment.
   */
  mov %cx, %ds
  /*
   * Enable a20.
   */
a20.1:
  in $0x64, %al
  test $2, %al
  jnz a20.1
  mov $0xd1, %al
  out %al, $0x64
a20.2:
  in $0x64, %al
  test $2, %al
  jnz a20.2
  mov $0xdf, %al
  out %al, $0x60
  /*
   * We can't use the vmm gdt yet: we have not access to the upper memory.
   * We use our own gdt until we reach the protected mode.
   * Update the gdt pointer (we didn't know where we are in memory).
   */
  lea gdt_start(%edx), %eax
  mov %eax, (gdt_desc + 2)
  lgdt gdt_desc
