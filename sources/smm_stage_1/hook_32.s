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
.code32
hook:

  // Save GPRs

  push %edi
  push %esi
  push %ebp
  push %ebx
  push %edx
  push %ecx
  push %eax

  // Load eip in esi for PIC purpose
  call ip
ip:
  pop %esi

  // Save segment selectors

  push %gs
  push %fs
  push %es
  push %ss
  push %ds
  push %cs

  // Save GTD desc

  sub $0x6, %esp
  sgdt (%esp)

  // Save cr4

  mov %cr4, %eax
  pushl $0x0
  push %eax

  // Save cr3

  mov %cr3, %eax
  pushl $0x0
  push %eax

  // Save cr0

  mov %cr0, %eax
  pushl $0x0
  push %eax

  // Load new GDT

  lgdt gdt_desc - ip(%esi)

  // Load new segments

  mov $0x20, %eax
  mov %ax, %ds
  mov %ax, %ss
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

  // Enable pae

  mov %cr4, %eax
  mov $0x20, %eax
  mov %eax, %cr4

  // Enable long mode (setup the EFER MSR)

  mov $0xc0000080, %ecx
  rdmsr
  or $0x00000100, %eax
  wrmsr

  // Set smm_stage_2 overall id mapping, see sbss symbol in smm_stage_2.elf
  // Page tables symbol is "pages".
  // It has to be id mapping to fit with efi current memory configuration
  mov 0x1005000, %eax
  mov %eax, %cr3

  // Enable paging.

  mov %cr0, %ebx
  or $0x80000000, %ebx
  mov %ebx, %cr0

  /*
   * Jump into 64 mode using far ret.
   * TODO: adjust 0x10 (depend on gdt).
   */
  leal long - ip(%esi), %ebx
  pushl $0x10
  pushl %ebx
  lret

.code64
long:

  sub $0x8, %rsp
  // See kernel_start symbol in smm_stage_2.elf
  movq $0x1000000, %rax
  push %rax
  // Call hook
  call *(%rsp)
  add $0x8, %rsp

  // Restore cr0

  pop %rax
  mov %rax, %cr0

  // Restore cr3

  pop %rax
  mov %rax, %cr3

  // Restore cr4

  pop %rax
  mov %rax, %cr4

  // Restore GDT desc

  lgdt (%esp)
  add $0x6, %esp

  // Restore code segment selector
  /*
   * Jump back into 32 mode using far ret.
   * TODO: adjust 0x10 (depend on gdt).
   */
  lea protected - ip(%esi), %rbx
  // old %s is already on the stack
  pushq %rbx
  lretq

.code32
protected:

  // Restore segment selectors
  pop %ds
  pop %ss
  pop %es
  pop %fs
  pop %gs

  // Restore GPRs

  pop %eax
  pop %ecx
  pop %edx
  pop %ebx
  pop %ebp
  pop %esi
  pop %edi

  rsm

/*
 * The gdt. This gdt must be closed to the one used by the vmm in
 * order to jump easily into the vmm code.
 * We define 64 and 32 segments although we use 64 segments only
 */
gdt_desc:
  /*
   * Size of gdt structure.
   */
  .word gdt_end - gdt_start - 1
  /*
   * Linear address of gdt.
   */
  .long gdt_start
gdt_start:
  /* 0x00: NULL */
  .long 0
  .long 0
  /* 0x08: Code 32 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x9a
  .byte 0xcf
  .byte 0
  /* 0x10: Code 64 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x9a
  .byte 0xaf
  .byte 0
  /* 0x18: Data 32 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x92
  .byte 0xcf
  .byte 0
  /* 0x20: Data 64 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x92
  .byte 0xaf
  .byte 0
gdt_end:
