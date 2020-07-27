.global hook
hook:

  // Save GPRs

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
  push %rbx
  push %rdx
  push %rcx
  push %rax

  sub $0x8, %rsp
  // Pushq $0x100000000
  movq $0x100000000, %rax
  pushq %rax
  // Call hook
  call *(%rsp)
  add $0x8, %rsp

  // Restore GPRs

  pop %rax
  pop %rcx
  pop %rdx
  pop %rbx
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

  rsm
