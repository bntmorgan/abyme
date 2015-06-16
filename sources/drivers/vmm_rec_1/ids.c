#include "stdio.h"
#include "hook.h"
#include "vmm.h"

const char *yolo = "SSTIC-AWESOME-ROCKS-BABE";

int cpuid_pre(struct registers *gr) {
  if (gr->rax == 0xaaaaaaaa) {
    INFO("Modifying rcx\n");
    gr->rcx = (uint64_t)yolo[15] << 56 | (uint64_t)yolo[14] << 48 |
      (uint64_t)yolo[13] << 40 | (uint64_t)yolo[12] << 32 |
      (uint64_t)yolo[11] << 24 | (uint64_t)yolo[10] << 16 |
      (uint64_t)yolo[9] << 8 | (uint64_t)yolo[8] << 0;
  }
  return 0;
}

void hook_main(void) {
  // Install some hooks
  hook_boot[EXIT_REASON_CPUID] = &cpuid_pre;
}
