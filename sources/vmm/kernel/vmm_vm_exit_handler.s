.text
.global vmm_vm_exit_handler

vmm_vm_exit_handler:
  /*
   * Save guest general purpose registers.
   */
  push %rax
  push %rcx
  push %rdx
  push %rbx
  sub $8, %rsp /* push %rsp */
  push %rbp
  push %rsi
  push %rdi
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  pushq $0x0 /* Will contain rip */

  /*
   * Call our VM-Exit handler.
   * It takes the guest general purpose registers as a parameter (see gpr64_t).
   */
  call vmm_handle_vm_exit

  /*
   * Restore guest general-purpose registers, possibly modified by our handler.
   */
  pop %r15 /* Pop rip */
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rdi
  pop %rsi
  pop %rbp
  add $8, %rsp /* pop %rsp */
  pop %rbx
  pop %rdx
  pop %rcx
  pop %rax

  /*
   * Resume guest execution.
   * TODO: error checking.
   */
  vmresume
