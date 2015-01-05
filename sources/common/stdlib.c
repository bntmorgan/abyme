#include "stdlib.h"
//#include "string.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

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

//XXX uint64_t is very big
/*uint64_t pow(uint64_t number, uint64_t p) {
  if (p == 0) {
    return 1;
  }
  uint64_t n = number;
  uint64_t i;
  for (i = 1; i < p; ++i) {
    n *= number;
  }
  return n;
}*/

/*uint64_t atoi_hexa(char *s) {
  unsigned int size = strlen(s), i;
  uint64_t number = 0;
  for (i = 0; i < size; ++i) {
    if (s[i] >= '0' && s[i] <= '9') {
      number += (s[i] - 0x30) * pow(0x10, size - i - 1);
    } else if(s[i] >= 'a' && s[i] <= 'f') {
      number += (s[i] - 'a' + 10) * pow(0x10, size - i - 1);
    }
    //printk("c:%c %X\n", s[i], number);
  }
  return number;
}*/

