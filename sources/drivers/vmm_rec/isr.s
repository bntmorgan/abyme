/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

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
  // error code not valid
  pushq $0x0
  pushq $\@
  callq interrupt_handler
  addq $0x18, %rsp
  // sti
  iretq
.else
  // cli
  // error code is valid
  pushq $0x1
  pushq $\@
  callq interrupt_handler
  addq $0x10, %rsp
  // sti
  iretq
.endif
.endm

.align 8
isr:
.rep 256
gen_isr_code
.endr
