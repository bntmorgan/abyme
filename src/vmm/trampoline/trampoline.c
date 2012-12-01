__asm__(".code16gcc\n");
// TODO: prepare stack
__asm__ ("jmpl  $0x100, $main\n");

#include <stdint.h>

#define Y               0
#define X               74
#define NB_COLUMNS      80

void __attribute__((noreturn)) main(void) {
  __asm__ __volatile__ (
      "mov $0xb800, %ax ;"
      "mov %ax, %ds     ;"
    );
  uint16_t *pos = (uint16_t *) ((X + Y * NB_COLUMNS) * sizeof(uint16_t));
  uint16_t number = 0;
  while (1) {
    /*
     * A 16 bits number contains 5 digits in decimal.
     */
    for (uint8_t j = 0; j < 5; j++) {
      uint16_t tmp = number;
      for (uint8_t k = 0; k < j; k++) {
        tmp = tmp / 10;
      }
      pos[5 - j] = (uint16_t) (((uint16_t) '0' + (tmp % 10))) | 0x3d00;
    }
    for (uint16_t k = 0; k < 0xf; k++)
      for (uint16_t i = 0; i < 0xffff; i++);
    number = number + 1;
  }
}
