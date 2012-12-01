#include "screen_int.h"

#include "string_int.h"

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding) {
  int8_t *ptr = string;
  uint8_t length = 0;
  while (*ptr) {
    length++;
    ptr++;
  }
  while (length < minimum_length) {
    scr_print(padding);
    length++;
  }
  while (*string) {
    scr_print((int8_t) *string);
    string++;
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
      scr_print(c);
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
      } else if (c == 's') {
        int8_t *string = __builtin_va_arg(values, int8_t *);
        printk_string(string, minimum_length, padding);
      } else if (c == 'c') {
        int32_t character = __builtin_va_arg(values, int32_t);
        scr_print((uint8_t) character);
      } else {
        scr_print(c);
      }
    }
    c = *format;
    format = format + 1;
  }
  __builtin_va_end(values);
}
