#include "types.h"
#include "seg.h"

__asm__("jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

typedef struct {
        unsigned int eax;
        unsigned int ebx __attribute__ ((packed));
        unsigned int ecx __attribute__ ((packed));
        unsigned int edx __attribute__ ((packed));
        unsigned int esi __attribute__ ((packed));
        unsigned int edi __attribute__ ((packed));
} SMMRegisters;

/* BIOS interrupts must be done with inline assembly */
void __NOINLINE __REGPARM print(const char *s) {
  static int i = 0;
  while (*s) {
    if (*s == '\r') {
      i -= i % 0xa0; // Caret return
    } else if (*s == '\n') {
      i += 0xa0; // New line
    } else {
      __asm__ __volatile__ (
          "push %%ds; mov %%cx, %%ds;" 
          //"xchg %%bx, %%bx;" 
          "mov %%ax, (%%bx);" 
          "pop %%ds" 
      : : "a"(0x0E00 | *s), "b"(i), "c"(0xb800));
      i += 2;
    }
    s++;
    // End of screen
    if (i >= (25 * 0xa0)) {
      i = 0;
    }
  }
}

void __NOINLINE __REGPARM clear_screen(char c) {
  uint32_t i = 0, size = 0xa0 * 25;
  for (i = 0; i < size; i++) {
    __asm__ __volatile__ (
        "push %%ds; mov %%cx, %%ds;" 
        "mov %%ax, (%%bx);" 
        "pop %%ds" 
    : : "a"(0x0000 | c), "b"(i), "c"(0xb800));
  }
}

void itoa(int8_t *dst, uint8_t base, int32_t value) {
  uint32_t uvalue = (uint32_t) value;
  int8_t *ptr = dst;
  /*
   * Set the sign for decimal values.
   */
  if (base == 10 && value < 0) {
    *dst = '-';
    dst = dst + 1;
    ptr = ptr + 1;
    uvalue = -value;
  }
  /*
   * Transform into string in reverse order.
   */
  do {
    *ptr = uvalue % base;
    if (*ptr < 10) {
      *ptr = *ptr + '0';
    } else {
      *ptr = *ptr + 'a' - 10;
    }
    ptr = ptr + 1;
    uvalue = uvalue / base;
  } while (uvalue > 0);
  *ptr = 0;
  ptr = ptr - 1;
  /*
   * Correct order of the string.
   */
  while (dst < ptr) {
    uint8_t tmp = *ptr;
    *ptr = *dst;
    *dst = tmp;
    dst = dst + 1;
    ptr = ptr - 1;
  }
}

#define PRINT_REG(buf, reg, value) \
  itoa(buf, 16, value); \
  print("reg : "); \
  print(reg); \
  print(" value :"); \
  print(buf); \
  print(" "); \

void print_regs(SMMRegisters *regs) {
  char buf[256];
  PRINT_REG(buf, "eax", regs->eax);
  PRINT_REG(buf, "ebx", regs->ebx);
  PRINT_REG(buf, "ecx", regs->ecx);
  PRINT_REG(buf, "edx", regs->edx);
  PRINT_REG(buf, "esi", regs->esi);
  PRINT_REG(buf, "edi", regs->edi);
  print("\r\n");
}

static int i8k_smm(SMMRegisters *regs) {
  int rc;
  int eax = regs->eax;

  print_regs(regs);

  asm("pushl %%eax\n\t" \
      "movl 0(%%eax),%%edx\n\t" \
      "push %%edx\n\t" \
      "movl 4(%%eax),%%ebx\n\t" \
      "movl 8(%%eax),%%ecx\n\t" \
      "movl 12(%%eax),%%edx\n\t" \
      "movl 16(%%eax),%%esi\n\t" \
      "movl 20(%%eax),%%edi\n\t" \
      "popl %%eax\n\t" \
      "out %%al,$0xb2\n\t" \
      "xchgl %%eax,(%%esp)\n\t"
      "movl %%ebx,4(%%eax)\n\t" \
      "movl %%ecx,8(%%eax)\n\t" \
      "movl %%edx,12(%%eax)\n\t" \
      "movl %%esi,16(%%eax)\n\t" \
      "movl %%edi,20(%%eax)\n\t" \
      "popl %%edx\n\t" \
      "movl %%edx,0(%%eax)\n\t" \
      "lahf\n\t" \
      "shrl $8,%%eax\n\t" \
      "andl $1,%%eax\n" \
    : "=a" (rc)
    : "a" (regs)
    : "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");

  print_regs(regs);

  if ((rc != 0) || ((regs->eax & 0xffff) == 0xffff) || (regs->eax == eax)) {
    return -1;
  }

  return 0;
}

int __NORETURN main(void) {
  clear_screen(' ');
  print("START LOL\r\n");
  SMMRegisters regs = { 0, 0, 0, 0, 0, 0 };
  regs.eax = 0x10a3;
  // 4 times to be shure
  i8k_smm(&regs);
  i8k_smm(&regs);
  i8k_smm(&regs);
  i8k_smm(&regs);
  print("END LOL\r\n");
  while (1);
}

