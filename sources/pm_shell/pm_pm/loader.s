.global loader

.set FLAGS,    0x4
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0
.long 0
.long 0
.long 0
.long 0

.long 1
.long 80
.long 25
.long 0

# reserve initial kernel stack space
stack_bottom:
.skip 16384                             # reserve 16 KiB stack
stack_top:

.comm mbd, 4                           # we will use this in kmain
.comm magic, 4                         # we will use this in kmain

loader:
    movl  $stack_top, %esp               # set up the stack, stacks grow downwards
    movl  %eax, magic                   # Multiboot magic number
    movl  %ebx, mbd                     # Multiboot data structure
    cli

    lgdt gdtr
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    jmpl $0x08, $main

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
