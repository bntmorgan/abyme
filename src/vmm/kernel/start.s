.text
.global start

/*
 * Declare the stack: 16384 bytes with an alignment of 32 (i.e. the least
 * significant 5 bits of the address should be zero.
 */
.set STACKSIZE, 0x4000

start:
  mov $(stack + STACKSIZE), %rsp
  mov %rsp, %rbp

  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %gs
  mov %ax, %fs
  mov %ax, %ss

  /*
   * The loader stored in edi the address of the vmm info struct.
   * We extend this address to 64 bits address.
   */
  xor %rax, %rax
  mov %edi, %eax
  mov %rax, %rdi
  call kernel_main
  /*
   * Disable interrupts, halt and infinite loop.
   */
  cli
  hlt
stop:
  jmp stop

stack: .zero STACKSIZE

