.set STACKSIZE, 0x10000
.comm stack,STACKSIZE,32
.text
.globl start
.type start, @function

start:
  /* TODO: verify the stack! */
  lea stack(%rip), %rax
  lea STACKSIZE(%rax), %rsp
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
