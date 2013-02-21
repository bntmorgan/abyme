.extern main

.code16
loader:
  xor    %ax, %ax
  mov    %ax, %ds
  mov    %ax, %ss
  mov    $0x9c00, %sp
  cli    
  push   %ds
  lgdt   gdtr
  mov    %cr0, %eax
  or     $0x1, %al
  mov    %eax, %cr0
  mov    $0x10, %bx
  mov    %bx, %ds
  jmpl   $0x8, $test

.code32
test: 
  xchg %bx, %bx
  jmp main

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
