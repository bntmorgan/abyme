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
   * Set the stack. The vmm will change the $0x50 in the code with a different
   * valu$0x0 in order to be thread safe for multi-core (the stacks must not
   * overlap).
   */
  mov $0x50, %ax
  mov %ax, %ss
  mov $0xfe, %sp
  /*
   * Change cs:ip in order to have ip = 0 for the first byte of this code.
   */
  xor %edx, %edx
  xor %ebx, %ebx
  call get_address
get_address:
  pop %bx
  /* edx == (cs << 4) */
  mov %cs, %dx
  shl $0x4, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] */
  add %ebx, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] - $get_address */
  sub $get_address, %edx
  /* edx == (new_cs << 4) */
  /* ecx == new_cs */
  mov %edx, %ecx
  shr $0x4, %ecx
  /* Simulate a long jump using long ret. */
  push %cx
  pushw $cs_adjusted
  lret
  /*
   * ecx and edx must never be changed!
   */
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
  /*
   * Go to protected mode.
   */
  mov %cr0, %eax
  or $0x1, %eax
  mov %eax, %cr0
  /*
   * Again, jump using a long return.
   */
  lea protected_mode(%edx), %ebx
  pushw $0x08
  push %bx
  lret
/*
 * The gdt. This gdt must be closed to the one used by the vmm in
 * order to jump easily into the vmm code.
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
  /* NULL */
  .long 0
  .long 0
  /* Code 32 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x9a
  .byte 0xcf
  .byte 0
  /* Code 64 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x9a
  .byte 0xaf
  .byte 0
  /* Data 32 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x92
  .byte 0xcf
  .byte 0
  /* Data 64 */
  .word 0xffff
  .word 0
  .byte 0
  .byte 0x92
  .byte 0xaf
  .byte 0
gdt_end:

.code32
protected_mode:
  /*
   * ecx can be used (real mode segmentation is not used in protected mode).
   * We still need edx (the position in memory): save into edi (rdmsr and other
   * instruction change edx!).
   */
  mov %edx, %edi
  /*
   * Update segment selectors.
   */
  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %ss
  mov $0x5fe, %esp
  /*
   * The vmm must have register the gdt pointer and cr3 just before
   * this code.
   */
  lgdt GDT(%edi)
  mov CR3(%edi), %eax
  mov %eax, %cr3
  /*
   * Enable pae.
   */
  mov $0x20, %eax
  mov %eax, %cr4
  /*
   * Enable long mode (setup the EFER MSR).
   */
  mov $0xc0000080, %ecx
  rdmsr
  or $0x00000100, %eax
  wrmsr
  /*
   * Enable paging.
   */
  mov %cr0, %ebx
  or $0x80000000, %ebx
  mov %ebx, %cr0
  /*
   * Jump into 64 mode using far ret.
   * TODO: adjust 0x10 (depend on gdt).
   */
  leal start64(%edi), %ebx
  pushl $0x10
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
  mov $0x5fe, %rsp
  /*
   * TODO: stack!
   */
  //lcall FCN(%edi)
  mov FCN(%edi), %rax
  call *%rax
end64:
  jmp end64
