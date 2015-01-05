#include "stdio.h"

#include "screen.h"
#include "stdlib.h"
//#include "keyboard.h"
#include "systab.h"

void (*putc)(uint8_t) = &screen_print;

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding) {
  int8_t *ptr = string;
  uint8_t length = 0;
  while (*ptr) {
    length++;
    ptr++;
  }
  while (length < minimum_length) {
    (*putc)(padding);
    length++;
  }
  while (*string) {
    (*putc)((int8_t) *string);
    string++;
  }
}

#ifndef _NO_PRINTK
void printk(char *format, ...) {
  __builtin_va_list values;
  __builtin_va_start(values, format);
  /*
   * At most, 64 characters needed to print a number with the lowest base (2).
   */
  int8_t buffer[65];
  int8_t c = *format;
  format = format + 1;
  while (c != 0) {
    if (c != '%') {
      (*putc)(c);
    } else {
      uint8_t minimum_length = 0;
      int8_t padding = (int8_t) ' ';
      /*
       * Read the minimum length if available.
       */
      c = *format;
      format = format + 1;
      if (c == '0') {
        padding = (int8_t) '0';
      }
      while ('0' <= c && c <= '9') {
        minimum_length = minimum_length * 10 + c - '0';
        c = *format;
        format = format + 1;
      }
      /*
       * Read the base and convert according to.
       */
      if (c == 'd') {
        int32_t value = __builtin_va_arg(values, int32_t);
        itoa(buffer, 10, value);
        printk_string(buffer, minimum_length, padding);
      } else if (c == 'x') {
        int32_t value = __builtin_va_arg(values, int32_t);
        itoa(buffer, 16, value);
        printk_string(buffer, minimum_length, padding);
#ifndef _CODE16GCC_
      } else if (c == 'X') {
        int64_t value = __builtin_va_arg(values, int64_t);
        uint8_t hi_minimum_length = 0;
        if (minimum_length > 8) {
          hi_minimum_length = minimum_length - 8;
        }
        if ((uint32_t) (value >> 32) > 0) {
          itoa(buffer, 16, (uint32_t) (value >> 32));
          printk_string(buffer, hi_minimum_length, padding);
        } else if (hi_minimum_length > 0) {
          itoa(buffer, 16, 0);
          printk_string(buffer, hi_minimum_length, padding);
        }
        uint8_t lo_minimum_length = minimum_length;
        if (minimum_length > 8) {
          lo_minimum_length = 8;
        }
        itoa(buffer, 16, (uint32_t) value);
        printk_string(buffer, lo_minimum_length, padding);
#endif
      } else if (c == 's') {
        int8_t *string = __builtin_va_arg(values, int8_t *);
        printk_string(string, minimum_length, padding);
      } else if (c == 'c') {
        int32_t character = __builtin_va_arg(values, int32_t);
        (*putc)((uint8_t) character);
      } else {
        (*putc)(c);
      }
    }
    c = *format;
    format = format + 1;
  }
  __builtin_va_end(values);
}
#else  /* _NO_PRINTK */
void printk(char *format, ...) {
  return;
}
#endif /* _NO_PRINTK */

void printk_bin(uint32_t size, char *sep, uint8_t *data) {
  uint32_t i, j;
  uint8_t d;
  for (i = size ; i > 0; i--) {
    d = *(data + i - 1);
    for (j = 8; j > 0; j--) {
      printk("%x", (d & 0x80) >> 7);
      d <<= 1;
    }
    printk("%s", sep);
  }
  printk("\n");
}
