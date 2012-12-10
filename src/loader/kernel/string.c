#include "string.h"

#include "include/types.h"

void memset(void *dst, uint8_t c, size_t size) {
  __asm__ __volatile__(
      "pushf     ;"
      "cld       ;"
      "rep movsb ;"
      "popf      ;"
    : : "D"(dst), "a"(c), "c"(size));
}

void memcpy(void *dst, void *src, size_t size) {
  __asm__ __volatile__(
      "pushf     ;"
      "cld       ;"
      "rep movsb ;"
      "popf      ;"
    : : "D"(dst), "S"(src), "c"(size));
}
