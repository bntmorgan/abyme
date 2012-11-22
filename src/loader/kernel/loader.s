#include "multiboot.h"
#include "kernel.h"

.text
.global loader

/*
 * Declare the stack: 16384 bytes with an alignment of 32 (i.e. the least
 * significant 5 bits of the address should be zero.
 */
.set STACKSIZE, 0x4000
.comm stack, STACKSIZE, 32

loader:
  movl $(stack + STACKSIZE), %esp
  pushl %ebx
  pushl %eax
  call kernel_main
  /*
   * Disable interrupts, halt and infinite loop.
   */
  cli
  hlt
stop:
  jmp stop
