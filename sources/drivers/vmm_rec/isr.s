/* filename: isr.s */
.globl   isr_wrapper
.align   4
 
isr_wrapper:
  cli
  sub $0x8, %rsp
  callq interrupt_handler
  add $0x8, %rsp
  sti
  iretq
