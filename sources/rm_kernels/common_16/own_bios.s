.global own_bios
.global run_protected

.extern hook_bios
.extern printk

.align 4

/**
 * Runs the pointed function in AWESOME protected mode
 * Save the caller segmentation
 * That rocks
 */
.code16
run_protected:
  push %ebp
  mov %esp, %ebp

  cli
  // We need a new gdt when we are with tinyvisor (test without failed : triple faulted)
  // Register our gtd
  lgdt gdtr
  // Go into the protected mode
  mov %cr0, %eax
  or $0x1, %al
  mov %eax, %cr0

  // Save the segmentation state
  push %ss
  push %cs
  push %ds
  push %es
  push %fs
  push %gs

  // Select the good segments for the gdt
  ljmp $0x08, $next

.code32
next:
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  
  // Calls the protected function
  // first parameter : address
  mov 8(%ebp), %eax
  // second parameter : parameter
  mov 12(%ebp), %ebx

  push %ebx
  call *%eax

  // Free memory
  add $0x4, %esp

  // Restore 16 bits segments
  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  ljmp $0x18, $end 

.code16  
end:
  // Go back to real mode dudes
  mov %cr0, %eax
  and $0xfffffffe, %ax
  mov %eax, %cr0

  // Restore caller segmentation
  pop %gs
  pop %fs
  pop %es
  pop %ds
  // pop %cs
  pop %ax 
  pop %ss

  // Create the seg:offset address for the long jump
  // push %cs
  push %ax
  push $very_end

  ljmp *(%esp)

.code16
very_end:
  pop %eax
  mov %ebp, %esp
  pop %ebp
  sti
  retl


.code32
own_bios:
  push %ebp
  mov %esp, %ebp

  /* unprotect the BIOS memory */
  // XXX Unprotecting the bios memory
  // Copying the BIOS flash in ram
  // See Intel 3rd generation core PAM0-PAM6
  movb $0x30, 0xf8000080
  movb $0x33, 0xf8000081
  movb $0x33, 0xf8000082
  movb $0x00, 0xf80f80d8

  /**
   * Get the first parameter
   * It is used to locate where to install
   * the bios hang
   */ 
  mov 8(%ebp), %eax
  mov %eax, handler_address

  // Save the handler
  cld
  mov handler_address, %esi
  mov $handler_save, %edi
  mov $(bioshang_end - bioshang_start), %ecx
  rep movsb

  // Own the handler
  cld
  mov $bioshang_start, %esi
  mov handler_address, %edi
  mov $(bioshang_end - bioshang_start), %ecx
  rep movsb

  mov %ebp, %esp
  pop %ebp
  ret

.code16
bioshang_start:

  // Save the things we need to be unchanged

  // %esp : %esp + 0xa
  pushl %esp
  // %eax : %esp + 0x6
  pushl %eax
  call _eip
_eip:
  popl %eax
  // %eip : %esp + 0x2
  pushl %eax
  // %ds : %esp + 0x0
  pushw %ds
  xor %ax, %ax
  mov %ax, %ds

  // Create the state structure in the hook stack 0x6000

  // Save %eax
  movl %ss:0x6(%esp), %eax
  movl %eax, 0x6000
  movl %ebx, 0x6004
  movl %ecx, 0x6008
  movl %edx, 0x600c
  // Save %esp
  movl %ss:0xa(%esp), %eax
  movl %eax, 0x6010
  movl %ebp, 0x6014
  movl %esi, 0x6018
  movl %edi, 0x601c
  // Save %eip
  movl %ss:0x2(%esp), %eax
  movl %eax, 0x6020
  // Save segments selectors
  // Save tr
  xor %ax, %ax
  movw %ax, 0x6024
  movw %gs, 0x6026
  movw %fs, 0x6028
  movw %es, 0x602a
  // Save %ds
  movw %ss:0x0(%esp), %ax
  movw %ax, 0x602c
  movw %ss, 0x602e
  movw %cs, 0x6030

  // Cleanup
  pop %ds
  pop %eax
  pop %eax
  pop %esp

  // Save the BIOS state in the current stack
  push %ss
  push %ds
  push %es
  push %fs
  push %gs
  push %esp
  push %ebp
  push %eax
  push %ebx
  push %ecx
  push %edx
  push %esi
  push %edi

  // Set up the hook_bios() environment
  xor %eax, %eax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss

  // Set our new stack
  movl $0x6000, %esp
  movl %esp, %ebp
  
  // Far call to us
  lcall $0x0, $hook_bios

  pushl $lulz
  // Far call to printk
  vmcall
  lcall $0x0, $printk
  
  // Save the BIOS state in the current stack
  pop %edi
  pop %esi
  pop %edx
  pop %ecx
  pop %ebx
  pop %eax
  pop %ebp
  pop %esp
  pop %gs
  pop %fs
  pop %es
  pop %ds
  // Of course ss at the end DUDE
  pop %ss

here:
  jmp here

bioshang_end:

/*
 * Descriptors.
 */
.macro GDT_ENTRY_16 type, base, limit
  .word (((\limit) >> 12) & 0xffff)
  .word (((\base)  >>  0) & 0xffff)
  .byte (0x00 + (((\base)  >> 16) & 0xff))
  .byte (0x90 + (((\type)  >>  0) & 0x6f))
  .byte (0x80 + (((\limit) >> 28) & 0x0f))
  .byte (0x00 + (((\base)  >> 24) & 0xff))
.endm

.macro GDT_ENTRY_32 type, base, limit
  .word (((\limit) >> 12) & 0xffff)
  .word (((\base)  >>  0) & 0xffff)
  .byte (0x00 + (((\base)  >> 16) & 0xff))
  .byte (0x90 + (((\type)  >>  0) & 0x6f))
  .byte (0xC0 + (((\limit) >> 28) & 0x0f))
  .byte (0x00 + (((\base)  >> 24) & 0xff))
.endm

gdt:
  GDT_ENTRY_32 0x0,                               0x0, 0x00000000
  GDT_ENTRY_32 0x8 /* SEG X */ + 0x2 /* SEG R */, 0x0, 0xffffffff
  GDT_ENTRY_32 0x2 /* SEG W */,                   0x0, 0xffffffff
  GDT_ENTRY_16 0x8 /* SEG X */ + 0x2 /* SEG R */, 0x0, 0xffffffff
  GDT_ENTRY_16 0x2 /* SEG W */,                   0x0, 0xffffffff
gdt_end:

gdtr:
  .word gdt_end - gdt - 1
  .long gdt

lulz:
  .byte 'a'
  .byte 'b'
  .byte 'c'
  .byte 0x0
 
handler_address:
  .long 0xcacacaca

handler_save:
  .space 0x100, 0xca

