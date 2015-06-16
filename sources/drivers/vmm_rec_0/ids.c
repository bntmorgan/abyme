#include "stdio.h"
#include "hook.h"
#include "vmm.h"

const char *yolo = "SSTIC-AWESOME-ROCKS-BABE";

int cpuid_pre(struct registers *gr) {
  if (gr->rax == 0xaaaaaaaa) {
    INFO("Modifying rbx\n");
    gr->rbx = (uint64_t)yolo[7] << 56 | (uint64_t)yolo[6] << 48 |
      (uint64_t)yolo[5] << 40 | (uint64_t)yolo[4] << 32 |
      (uint64_t)yolo[3] << 24 | (uint64_t)yolo[2] << 16 |
      (uint64_t)yolo[1] << 8 | (uint64_t)yolo[0] << 0;
  }
  return 0;
}

void hook_main(void) {
  // Install some hooks
  hook_boot[EXIT_REASON_CPUID] = &cpuid_pre;
}
