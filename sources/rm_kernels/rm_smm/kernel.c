#include "types.h"
#include "seg.h"

__asm__("jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

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

//#define DEBUG

int __NORETURN main(void) {
  // Sector read
  print("START LOL\r\n");
  while (1);
}

