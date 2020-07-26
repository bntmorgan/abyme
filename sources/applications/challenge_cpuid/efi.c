#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "msr.h"
#include "cpu.h"
#include "shell.h"

static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;

uint64_t env_tsc_to_micro(uint64_t t) {
  return t / tsc_freq_MHz;
}

// CPUID  O - f
//        80000000 - 8000008
void challenge_start(void) {
  uint64_t rax, rbx, rcx, rdx;
  uint32_t i, j;
  uint64_t xor[4];
  for (j = 0; j < 0x100; j++) {
    // init
    xor[0] = 0, xor[1] = 0, xor[2] = 0, xor[3] = 0;
    // 0 to f
    for (i = 0; i < 0x10; i++) {
      rax = i, rbx = 0x0, rcx = 0x0, rdx = 0x0;
      __asm__ __volatile__("cpuid" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) :
          "a"(rax), "b"(rbx), "c"(rcx), "d"(rdx));
      xor[0] ^= rax, xor[1] ^= rbx, xor[2] ^= rcx, xor[3] ^= rdx;
    }
    // 80000000 - 80000008
    for (i = 0x80000000; i < 0x9; i++) {
      rax = i, rbx = 0x0, rcx = 0x0, rdx = 0x0;
      __asm__ __volatile__("cpuid" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx) :
          "a"(rax), "b"(rbx), "c"(rcx), "d"(rdx));
      xor[0] ^= rax, xor[1] ^= rbx, xor[2] ^= rcx, xor[3] ^= rdx;
    }
  }
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);

  // Print to shell
  putc = &shell_print;

  uint64_t micros, a, b;
  // Init tsc
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;

  a = cpu_rdtsc();
  challenge_start();
  b = cpu_rdtsc();

  micros = env_tsc_to_micro(b - a);
  INFO("Challenge execution time : 0x%016X microseconds\n", micros);

  return EFI_SUCCESS;
}
