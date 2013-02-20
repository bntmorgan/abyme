#include "types.h"

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

/* BIOS interrupts must be done with inline assembly */
void __NOINLINE __REGPARM print(const char *s) {
  static uint8_t i = 0;
  while (*s) {
    *((uint8_t *)(0xb8000 +  2 * i)) = *s;
    s++;
    i++;
  }
}

//#define DEBUG
int __NORETURN main(void) {
  // Sector read
  print("bonjour\n");
  while (1);
}

