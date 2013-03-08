#include "string.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

void memset(void *dst, uint8_t c, size_t size) {
  __asm__ __volatile__(
      "pushf     ;"
      "cld       ;"
      "rep stosb ;"
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

unsigned int strlen(char *s) {
  unsigned int size = 0, i;
  for (i = 0; s[i] != '\0'; ++i) {
    ++size;
  }
  return size;
}
