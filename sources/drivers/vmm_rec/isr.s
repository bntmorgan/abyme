/* filename: isr.s */

.global isr

/* XXX directly call interrupt handler and pop the right number arguments */
.macro gen_isr_code 
.global _isr\@
.align 32
_isr\@:
/* Fake error code : Add .id when already have */
// XXX no cli and sti : interruptions are already masked
.if (\@ - 8 && \@ - 10 && \@ - 11 && \@ - 12 && \@ - 13 && \@ - 14 && \@ - 17)
  // cli 
  pushq $0x0
  pushq $\@
  callq interrupt_handler 
  addq $0x10, %rsp
  // sti
  iretq
.else
  // cli
  pushq $\@
  callq interrupt_handler 
  addq $0x8, %rsp
  // sti
  iretq
.endif
.endm

.align 8
isr:
.rep 256
gen_isr_code
.endr
