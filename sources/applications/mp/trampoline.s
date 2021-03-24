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
.code16
start16:
  jmp start16_cont

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
  call get_address
get_address:
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
  /* rip relative cs_adjusted address */
  add $(cs_adjusted - get_address), %ax
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
  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt ap_param - cs_adjusted
  /*
   * Go to protected mode.
   */
  mov %cr0, %eax
  or $0x1, %eax
  mov %eax, %cr0
  /*
   * Again, jump using a long return.
   */
  /* Get protected_mode address, cs_adjusted rip is zero !*/
  mov $(protected_mode - cs_adjusted), %ebx
  /* Prepare lret stack */
  pushw $0x8
  push %bx
  lret

.global protected_mode
protected_mode:
  /*
   * Update segment selectors.
   */
  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %ss

  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt ap_param - protected_mode + 0x06

  /* CR3 */
  movl ap_param - protected_mode + 0x10, %ebx
  movl %ebx, %cr3

  /*
   * Request long mode (setup the EFER MSR).
   */
  mov $0xc0000080, %ecx
  rdmsr
  or $0x00000100, %eax
  wrmsr
  /*
   * Enable paging (and so long mode)
   */
  mov %cr0, %ebx
  or $0x80000000, %ebx
  mov %ebx, %cr0
  /*
   * Jump into 64 mode using far ret.
   * TODO: adjust 0x10 (depend on gdt).
   */
  /* Get protected_mode address */
  pushl $(start64 - protected_mode)
  pushl %ebx
  lret

.code64
start64:
  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %gs
  mov %ax, %fs
  mov %ax, %ss
  mov %ax, %ss
  callq get_address_lm
get_address_lm:
  pop %rax
  /* Get vmm_next address */
  add $(ap_param - get_address_lm + 0x14), %rax
  /* jumps into the vmm_next function ! */
  callq *%rax
end:
  jmp end

.global ap_param
ap_param: 
  /* AP params */
  /* sizeof struct ap_param */
  .long 0
  .long 0
  .long 0
  .long 0
  .long 0
  .long 0
  .long 0


trampoline_end:
.global trampoline_end
