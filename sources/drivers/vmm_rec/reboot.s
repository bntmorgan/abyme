.global soft_reboot
.global soft_reboot_end

/**
 * Software reboot the machine from long mode outside VMX operation
 * This code is position independent and
 * it must be loaded for convenience in a < 0xffff address
 *
 * A code segment selector for a IA-32e legacy execution mode segment descriptor
 * needs to be pushed as the first argument of soft_reboot function.
 *
 * This code needs to be called in long mode i.e. return adress is a quad word
 * on the stack
 */

soft_reboot:

  /* To be shure but already set in sources/drivers/vmm_rec/vmexit.s */
  cli

rip:
  lea -0x7(%rip),%rax

  /* This code needs to be loaded <= 0xffff */

  xor %rbx, %rbx
  mov $0xffff, %bx
  cmpq %rbx, %rax
  jbe rip_ok

  /* Loaded to the wrong address space, unsupported */
  retq

rip_ok:

  /* Go to ia-32e legacy mode */
  /* The code legacy segment selector is the first argument of soft_reboot
   * function */
  xor %rbx, %rbx
  movw 8(%rsp), %bx
  pushq %rbx
  /* legacy address */
  mov %rax, %rbx
  add $(legacy - rip), %rbx
  pushq %rbx
  lretq

legacy:

/* The following instruction are available both in protected and long modes */

.code32
load_gdtr:

  /* Prepare protected mode environment */

  /* Load a 32-bit GDT */

  /* @gdtr */
  mov %eax, %ecx
  add $(gdtr - rip), %ecx

  /* gdtr.base */
  mov %eax, %ebx
  add $(gdt - rip), %ebx

  /* set gdtr.base */
  movl %ebx, 0x2(%ecx)

  lgdt (%ecx)

protected_mode_prepare:
  /* Load the 32-bit segments selectors */
  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %ss
  mov %ax, %gs
  mov %ax, %fs
  mov %ax, %es

  /* Go back to protected mode */

  mov %cr0, %ebx
  and $0x7fffffff, %ebx
  mov %ebx, %cr0

  /* compute protected_mode address */
  mov %eax, %ebx
  add $(protected_mode - rip), %ebx

  /* compute protected_mode_ptr_address */
  mov %eax, %ecx
  add $(protected_mode_ptr - rip), %ecx

  /* copy protected_mode address in the long jump ptr */
  mov %ebx, 0x2(%ecx)

  ljmpl *(%ecx)

protected_mode:

  /* Save rip */
  mov %eax, %ebx

  /* Reset IA32_EFER MSR */

  xor %eax, %eax
  xor %edx, %edx
  mov $0xc0000080, %ecx
  wrmsr

  /* restore rip */
  mov %ebx, %eax

  /* Prepare real mode environment */

  mov	$0x18, %cx
  mov	%cx, %ds
  mov	%cx, %es
  mov	%cx, %fs
  mov %cx, %gs
  mov	%cx, %ss

  /* compute real_mode_prepare address */
  mov %eax, %ebx
  add $(real_mode_prepare - rip), %ebx

  /* compute real_mode_prepare_ptr address */
  mov %eax, %ecx
  add $(real_mode_prepare_ptr - rip), %ecx

  /* copy real_mode_prepare address in the long jump ptr */
  mov %ebx, 0x2(%ecx)

  ljmpl *(%ecx)

  /* TODO set a real mode IDT ? */

.code16
real_mode_prepare: /* from linux arch/x86/realmode/rm/reboot.S */
  xorl %ecx, %ecx
  movl %cr0, %edx
  andl $0x00000011, %edx
  orl	$0x60000000, %edx
  movl %edx, %cr0
  movl %ecx, %cr3
  movl %cr0, %edx
  testl	$0x60000000, %edx	/* If no cache bits -> no wbinvd */
  jz no_wbinvd
  wbinvd
no_wbinvd:
  andb $0x10, %dl
  movl %edx, %cr0 /* end from */

  /* compute real_mode address */
  mov %eax, %ebx
  add $(real_mode - rip), %ebx

  /* compute real_mode_ptr address */
  mov %eax, %ecx
  add $(real_mode_ptr - rip), %ecx

  /* copy real_mode address in the long jump ptr */
  movw %bx, 0x2(%ecx)

  ljmpw *(%ecx)

real_mode:
  andw	%ax, %ax /* why ? */

the_end:
  /* Reboot the core */
  ljmpl $0xf000, $0xfff0

/**
 * Data
 */

/*
 * Descriptors
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
  .word gdt_end - gdt - 1 /* limit */
  .long 0xcafebabe /* base */

protected_mode_ptr:
  .word 0x0008
  .long 0xcafebabe

real_mode_prepare_ptr:
  .word 0x0008
  .long 0xcafebabe

real_mode_ptr:
  .word 0x0020
  .word 0xbabe

soft_reboot_end:
