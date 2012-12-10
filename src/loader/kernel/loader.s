#include "kernel.h"

.text
.global loader

/*
 * Declare the stack: 0x4000 bytes with an alignment of 32.
 */
.set STACKSIZE, 0x4000
.comm stack, STACKSIZE, 32

loader:
  movl $(stack + STACKSIZE), %esp
  /*
   * As mentionned in the multiboot specification, just before the invocation
   * of the 32 bits operating system, eax is initialized with the magic value
   * and ebx is initialized with the physical address of the multiboot
   * information structure.
   * See Multiboot Specification version 0.6.96, section 3.2.
   */
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
