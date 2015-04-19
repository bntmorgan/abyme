/* filename: isr.s */

.global isr

.macro gen_isr_code 
.global _isr\@
.align 16
_isr\@:
/* Fake error code : Add .id when already have */
.if \@ - 8
.if \@ - 10
.if \@ - 11
.if \@ - 12
.if \@ - 13
.if \@ - 14
.if \@ - 17
  pushq $0x0
.endif
.endif
.endif
.endif
.endif
.endif
.endif
  pushq $\@
  jmp isr_wrapper
.endm

.align 8
isr:
.rep 256
gen_isr_code
.endr

isr_wrapper:
  callq interrupt_handler 
  /* remove int number */
  addq $0x10, %rsp
  sti
  iretq
