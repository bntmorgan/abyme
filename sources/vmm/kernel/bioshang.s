.global bioshang_start
.global bioshang_end

.code16
bioshang_start:
  mov $0xb800, %ax
  mov %ax, %ds
  movw $0x764, 0x0
  movw $0x765, 0x2
  movw $0x766, 0x4
here:
  jmp here
bioshang_end:
