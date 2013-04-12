/*
 * See [Cpuid_May_2012].
 */

#include "cpuid.h"

#include "cpu.h"
#include "stdio.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

#define cpuid_a(idx, eax) \
  __asm__ __volatile__("cpuid" : "=a"(eax) : "a"(idx))
#define cpuid_cd(idx, ecx, edx) \
  __asm__ __volatile__("cpuid" : "=c"(ecx), "=d"(edx) : "a"(idx))
#define cpuid_abcd(idx, eax, ebx, ecx, edx) \
  __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(idx))

#define CPUID_0XX1_ECX         \
  HANDLE(sse3,          0,  0) \
  HANDLE(pclmuldq,      1,  1) \
  HANDLE(dtes64,        2,  2) \
  HANDLE(monitor,       3,  3) \
  HANDLE(ds_cpl,        4,  4) \
  HANDLE(vmx,           5,  5) \
  HANDLE(smx,           6,  6) \
  HANDLE(eist,          7,  7) \
  HANDLE(tm2,           8,  8) \
  HANDLE(ssse3,         9,  9) \
  HANDLE(cnxt_id,      10, 10) \
  HANDLE(reserved_1,   11, 11) \
  HANDLE(fma,          12, 12) \
  HANDLE(cx16,         13, 13) \
  HANDLE(xtpr,         14, 14) \
  HANDLE(pdcm,         15, 15) \
  HANDLE(reserved_2,   16, 16) \
  HANDLE(pcid,         17, 17) \
  HANDLE(dca,          18, 18) \
  HANDLE(sse4_1,       19, 19) \
  HANDLE(sse4_2,       20, 20) \
  HANDLE(x2apic,       21, 21) \
  HANDLE(movbe,        22, 22) \
  HANDLE(popcnt,       23, 23) \
  HANDLE(tsc_deadline, 24, 24) \
  HANDLE(aes,          25, 25) \
  HANDLE(xsave,        26, 26) \
  HANDLE(osxsave,      27, 27) \
  HANDLE(avx,          28, 28) \
  HANDLE(f16c,         29, 29) \
  HANDLE(rdrand,       30, 30) \
  HANDLE(not_used_1,   31, 31)

#define CPUID_0XX1_EDX       \
  HANDLE(fpu,         0,  0) \
  HANDLE(vme,         1,  1) \
  HANDLE(de,          2,  2) \
  HANDLE(pse,         3,  3) \
  HANDLE(tsc,         4,  4) \
  HANDLE(msr,         5,  5) \
  HANDLE(pae,         6,  6) \
  HANDLE(mce,         7,  7) \
  HANDLE(cx8,         8,  8) \
  HANDLE(apic,        9,  9) \
  HANDLE(reserved_1, 10, 10) \
  HANDLE(sep,        11, 11) \
  HANDLE(mtrr,       12, 12) \
  HANDLE(pge,        13, 13) \
  HANDLE(mca,        14, 14) \
  HANDLE(cmov,       15, 15) \
  HANDLE(pat,        16, 16) \
  HANDLE(pse_36,     17, 17) \
  HANDLE(psn,        18, 18) \
  HANDLE(clfsh,      19, 19) \
  HANDLE(reserved_2, 20, 20) \
  HANDLE(ds,         21, 21) \
  HANDLE(acpi,       22, 22) \
  HANDLE(mmx,        23, 23) \
  HANDLE(fxsr,       24, 24) \
  HANDLE(sse,        25, 25) \
  HANDLE(sse2,       26, 26) \
  HANDLE(ss,         27, 27) \
  HANDLE(htt,        28, 28) \
  HANDLE(tm,         29, 29) \
  HANDLE(reserved_3, 30, 30) \
  HANDLE(pbe,        31, 31)

#define CPUID_8XX1_EDX       \
  HANDLE(reserved_1, 0,  10) \
  HANDLE(syscall,    11, 11) \
  HANDLE(reserved_2, 12, 19) \
  HANDLE(xd_bit,     20, 20) \
  HANDLE(reserved_3, 21, 25) \
  HANDLE(page1g,     26, 26) \
  HANDLE(rdtscp,     27, 27) \
  HANDLE(reserved_4, 28, 28) \
  HANDLE(intel_64,   29, 29) \
  HANDLE(reserved_5, 30, 31)

#define CPUID_8XX8_EAX       \
  HANDLE(maxphyaddr, 0,   7) \
  HANDLE(maxlinaddr, 8,  15)

#define HANDLE(a, b, c) uint32_t a;

typedef union {
  struct {
    CPUID_0XX1_ECX
  } intel;
} __attribute__((packed)) cpuid_0xx1_ecx_t;

typedef union {
  struct {
    CPUID_0XX1_EDX
  } intel;
} __attribute__((packed)) cpuid_0xx1_edx_t;

typedef union {
  struct {
    CPUID_8XX1_EDX
  } intel;
} __attribute__((packed)) cpuid_8xx1_edx_t;

typedef union {
  struct {
    CPUID_8XX8_EAX
  } intel;
} __attribute__((packed)) cpuid_8xx8_eax_t;

#undef HANDLE

uint32_t cpuid_largest_extended_function;
uint32_t cpuid_largest_function;
uint8_t cpuid_vendor[13];
cpuid_8xx8_eax_t cpuid_8xx8_eax;
cpuid_8xx1_edx_t cpuid_8xx1_edx;
cpuid_0xx1_edx_t cpuid_0xx1_edx;
cpuid_0xx1_ecx_t cpuid_0xx1_ecx;

void cpuid_print_8xx8_eax(void) {
#define HANDLE(a, b, c) INFO("cpuid 8xx8 eax %12s[%02d:%02d]=%x\n", #a, b, c, cpuid_8xx8_eax.intel.a);
  CPUID_8XX8_EAX
#undef HANDLE
}

void cpuid_read_8xx8_eax(uint32_t value) {
#define HANDLE(a, b, c) cpuid_8xx8_eax.intel.a = (value >> b) & ((1 << (c - b + 1)) - 1);
  CPUID_8XX8_EAX
#undef HANDLE
}

void cpuid_print_0xx1_ecx(void) {
#define HANDLE(a, b, c) INFO("cpuid 0xx1 ecx %12s[%02d:%02d]=%x\n", #a, b, c, cpuid_0xx1_ecx.intel.a);
  CPUID_0XX1_ECX
#undef HANDLE
}

void cpuid_read_0xx1_ecx(uint32_t value) {
#define HANDLE(a, b, c) cpuid_0xx1_ecx.intel.a = (value >> b) & ((1 << (c - b + 1)) - 1);
  CPUID_0XX1_ECX
#undef HANDLE
}

void cpuid_print_0xx1_edx(void) {
#define HANDLE(a, b, c) INFO("cpuid 0xx1 edx %12s[%02d:%02d]=%x\n", #a, b, c, cpuid_0xx1_edx.intel.a);
  CPUID_0XX1_EDX
#undef HANDLE
}

void cpuid_read_0xx1_edx(uint32_t value) {
#define HANDLE(a, b, c) cpuid_0xx1_edx.intel.a = (value >> b) & ((1 << (c - b + 1)) - 1);
  CPUID_0XX1_EDX
#undef HANDLE
}

void cpuid_print_8xx1_edx(void) {
#define HANDLE(a, b, c) INFO("cpuid 8xx1 edx %12s[%02d:%02d]=%x\n", #a, b, c, cpuid_8xx1_edx.intel.a);
  CPUID_8XX1_EDX
#undef HANDLE
}

void cpuid_read_8xx1_edx(uint32_t value) {
#define HANDLE(a, b, c) cpuid_8xx1_edx.intel.a = (value >> b) & ((1 << (c - b + 1)) - 1);
  CPUID_8XX1_EDX
#undef HANDLE
}

uint8_t cpuid_has_local_apic(void) {
  return cpuid_0xx1_edx.intel.apic;
}

uint8_t cpuid_is_long_mode_supported(void) {
  return cpuid_8xx1_edx.intel.intel_64;
}

uint8_t cpuid_is_page1g_supported(void) {
  return cpuid_8xx1_edx.intel.page1g;
}

uint8_t cpuid_is_pae_supported(void) {
  return cpuid_0xx1_edx.intel.pae;
}

uint8_t cpuid_is_vmx_supported(void) {
  return cpuid_0xx1_ecx.intel.vmx;
}

uint8_t cpuid_get_maxphyaddr(void) {
  return cpuid_8xx8_eax.intel.maxphyaddr;
}

void cpuid_setup(void) {
  uint32_t tmp_a;
//  uint32_t tmp_b;
//  uint32_t tmp_c;
//  uint32_t tmp_d;
/*TODO
  cpuid_abcd(0x00000000, cpuid_largest_function,
      *((uint32_t *) (cpuid_vendor + 0)),
      *((uint32_t *) (cpuid_vendor + 8)),
      *((uint32_t *) (cpuid_vendor + 4)));
  cpuid_vendor[12] = 0;
  INFO("cpuid vendor: '%s'\n", cpuid_vendor);
  if (*((uint32_t *) (cpuid_vendor + 0)) != 0x756e6547 ||
      *((uint32_t *) (cpuid_vendor + 8)) != 0x6c65746e ||
      *((uint32_t *) (cpuid_vendor + 4)) != 0x49656e69) {
    INFO("Intel support only\n");
  }
  cpuid_a(0x80000000, cpuid_largest_extended_function);
  INFO("cpuid_largest_extended_function=%x\n", cpuid_largest_extended_function);
  if (cpuid_largest_extended_function == 0x80000000) {
    INFO("No enought features supported\n");
  }
*/
  cpuid_a(0x80000008, tmp_a);
  cpuid_read_8xx8_eax(tmp_a);
  cpuid_print_8xx8_eax();
/*  cpuid_cd(0x80000001, tmp_c, tmp_d);
  cpuid_read_8xx1_edx(tmp_d);
  cpuid_print_8xx1_edx();
  cpuid_abcd(0x00000001, tmp_a, tmp_b, tmp_c, tmp_d);
  cpuid_read_0xx1_edx(tmp_d);
  cpuid_read_0xx1_ecx(tmp_c);
  cpuid_print_0xx1_edx();
  cpuid_print_0xx1_ecx();
*/
}

uint8_t cpuid_are_mtrr_supported(void) {
  uint64_t rdx;
  __asm__ __volatile__(
    "cpuid;"
  : "=d"(rdx) : "a"(0x1));
  INFO("rdx : %x, bit 12 support ? %d\n", rdx, (rdx >> 12) & 0x1);
  return (rdx >> 12) & 0x1;
}

