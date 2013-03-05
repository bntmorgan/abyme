.global loader

.set FLAGS,    0
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# reserve initial kernel stack space
stack_bottom:
.skip 16384                             # reserve 16 KiB stack
stack_top:

.comm mbd, 4                           # we will use this in kmain
.comm magic, 4                         # we will use this in kmain

loader:
    movl  $stack_top, %esp              # set up the stack, stacks grow downwards
    movl  %eax, magic                   # Multiboot magic number
    movl  %ebx, mbd                     # Multiboot data structure
    cli

    /* Copy the bootstrap. */
    cld
    mov $bootstrap_start, %esi
    mov $0x600, %edi
    mov $(bootstrap_end - bootstrap_start), %ecx
    rep movsb

    /* Copy the kernel. */
    cld
    mov $kernel_start, %esi
    mov $0x700, %edi
    /* Note: The linker isn't able to compute $(kernel_end - kernel_start) */
    mov $(kernel_end), %ecx
    sub $(kernel_start), %ecx
    rep movsb
    
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
    
    lgdt gdtr
    mov $0x20, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    jmpl $0x18, $0x600

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

.code16
bootstrap_start:
    mov %cr0, %eax
    and $0xfffffffe, %eax
    mov %eax, %cr0
    xor %eax, %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $0x2000, %sp
    jmpl $0x0, $0x700
bootstrap_end:

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
