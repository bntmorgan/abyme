#include "stdio.h"

#include "screen.h"
#include "stdlib.h"
#include "keyboard.h"

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding) {
  int8_t *ptr = string;
  uint8_t length = 0;
  while (*ptr) {
    length++;
    ptr++;
  }
  while (length < minimum_length) {
    screen_print(padding);
    length++;
  }
  while (*string) {
    screen_print((int8_t) *string);
    string++;
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
      screen_print(c);
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
      } else if (c == 's') {
        int8_t *string = __builtin_va_arg(values, int8_t *);
        printk_string(string, minimum_length, padding);
      } else if (c == 'c') {
        int32_t character = __builtin_va_arg(values, int32_t);
        screen_print((uint8_t) character);
      } else {
        screen_print(c);
      }
    }
    c = *format;
    format = format + 1;
  }
  __builtin_va_end(values);
}

#define DEBUG_DUMP_COLUMNS 6
void dump(uint64_t addr, uint64_t size) {
  uint32_t i = 0, j;
  while (i < size) {
    printk("%016x ", addr + i);
    for (j = 0; j < DEBUG_DUMP_COLUMNS; j++) {
      intptr_t test = (addr + i);
      uint32_t lulz = *((uint32_t*)test);
      printk("%08x", lulz);
      if (i != DEBUG_DUMP_COLUMNS - 1) {
        printk(" ");
      }
      i += 4;
      if (i >= size) {
        break;
      }
    }
    printk("\n");
  }
}

int getchar() {
  char c = keyboard_decode(waitkey());
  printk("%c", c);
  return c;
}
