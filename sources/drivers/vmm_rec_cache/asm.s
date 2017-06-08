.global asm_probe

asm_probe:
  // Set
  mov %rax, %r9
  // Set size
  mov %rbx, %r10
  // Candidate
  mov %rcx, %r11
  mfence
  lfence
  // Load candidate cache line
  mov (%r11), %rax
  // for (i = 0; i < ss; i++)
  // i = 0
  xor %r12, %r12
  // Last line =0
  xor %r15, %r15
loop:
  cmp %r12, %r10
  jz endloop
  // %rax = set[i]
  mov (%r9, %r12, 8), %rax
  mfence
  lfence
  lea (%r9, %r12, 8), %r15
  mov (%rax), %rax
  // i++
  inc %r12
  jmp loop
endloop:
  // %r12 = rdtscp
  mfence
  lfence
  rdtscp
  mov %rax, %r12
  shl $32, %rdx
  or %rdx, %r12
  // Access candidate cache line
  mov (%r11), %rax
  // %r13 = rdtscp
  rdtscp
  mov %rax, %r13
  shl $32, %rdx
  or %rdx, %r13
  sub %r12, %r13
  mov %r13, %rax
  mov %r15, %rcx
  retq
