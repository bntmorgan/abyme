.text
.global vmm_vm_exit_handler

vmm_vm_exit_handler:
  // XXX We have now an invalid IDT !!!
  // Very important, if not, BSP core will be stalled in VM with a l0, l1
  // configuration
  cli
  /*
   * Save guest general purpose registers.
   */
  sub $8, %rsp /* placeholder, will be replaced by rip */
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rbp
  sub $8, %rsp /* placeholder, will be replaced by rsp */
  push %rbx
  push %rdx
  push %rcx
  push %rax

  /*
   * Call our VM-Exit handler.
   * It takes the guest general purpose registers as a parameter (see gpr64_t).
   */
  call vmm_handle_vm_exit

  /*
   * Restore guest general-purpose registers, possibly modified by our handler.
   */
  pop %rax
  pop %rcx
  pop %rdx
  pop %rbx
  add $8, %rsp /* pop %rsp */
  pop %rbp
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  add $8, %rsp /* pop %rip */

  /*
   * Resume guest execution.
   */
  // XXX
  sti
  vmresume

  /* Check errors */
  setc %al    /* VMfailInvalid : CF = 1 */
  setz %dl    /* VMfailValid : ZF = 1 */

  /* sysv_abi calling convention */
  mov %eax, %edi
  mov %edx, %esi
  call vmx_transition_display_error
