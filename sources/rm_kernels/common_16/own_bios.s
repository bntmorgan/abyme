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

.code32
restore_bios:
  push %ebp
  mov %esp, %ebp

  // Own the handler
  cld
  mov $handler_save, %esi
  mov 8(%ebp), %eax
  mov %eax, %edi
  mov $(bioshang_end - bioshang_start), %ecx
  rep movsb

  mov %ebp, %esp
  pop %ebp
  ret


.code16
bioshang_start:

  cli

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

  // Create the state structure 

  // Save %eax
  movl %ss:0x6(%esp), %eax
  movl %eax, bios_state + 0x00
  movl %ebx, bios_state + 0x04
  movl %ecx, bios_state + 0x08
  movl %edx, bios_state + 0x0c
  // Save %esp
  movl %ss:0xa(%esp), %eax
  movl %eax, bios_state + 0x10
  movl %ebp, bios_state + 0x14
  movl %esi, bios_state + 0x18
  movl %edi, bios_state + 0x1c
  // Save %eip
  movl handler_address, %eax
  movl %eax, bios_state + 0x20
  // Save segments selectors
  // Save tr
  // str bios_state + 0x24
  movw %gs, bios_state + 0x26
  movw %fs, bios_state + 0x28
  movw %es, bios_state + 0x2a
  // Save %ds
  movw %ss:0x0(%esp), %ax
  movw %ax, bios_state + 0x2c
  movw %ss, bios_state + 0x2e
  movw %cs, bios_state + 0x30

  // Cleanup
  pop %ds
  pop %eax
  pop %eax
  pop %esp

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
  
  // Set the parameter core_gpr pointer
  movl $bios_state, (%esp)

  // Far call to us
  ljmp $0x0, $call_hook_bios

bioshang_end:

.code16
call_hook_bios:
  // Call hook_bios

  calll hook_bios
  // Restore handler code

  // Second argument : handler address
  movl handler_address, %eax
  pushl %eax
  // First argument : function address
  pushl $restore_bios
  calll run_protected
  // ljmp handler_address

  // Create the seg:offset addr
  movl handler_address, %eax
  and $0x0000ffff, %eax
  movl handler_address, %ebx
  and $0xffff0000, %ebx
  shl $0xc, %ebx
  or %ebx, %eax
  movl %eax, handler_address

  // Restore core state 
  
  movl bios_state + 0x00, %eax
  movl bios_state + 0x04, %ebx
  movl bios_state + 0x08, %ecx
  movl bios_state + 0x0c, %edx
  movl bios_state + 0x10, %esp
  movl bios_state + 0x14, %ebp
  movl bios_state + 0x18, %esi
  movl bios_state + 0x1c, %edi
  // Restore segments selectors
  // ltr bios_state + 0x24
  movw bios_state + 0x26, %gs
  movw bios_state + 0x28, %fs
  movw bios_state + 0x2a, %es
  movw bios_state + 0x2e, %ss
  movw bios_state + 0x2c, %ds

  sti

  // Back again in bios hell
  ljmp %cs:*handler_address
  
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

handler_address:
  .long 0xcacacaca

bios_state:
  .space 50, 0xca

handler_save:
  .space 0x100, 0xca

