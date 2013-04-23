#include "cpu.h"

void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__(
      "out %%al, %%dx;"
  : : "a"(value), "d"(port));
}

void cpu_outportw(uint32_t port, uint16_t value) {
  __asm__ __volatile__(
      "out %%ax, %%dx;"
  : : "a"(value), "d"(port));
}

void cpu_outportd(uint32_t port, uint32_t value) {
  __asm__ __volatile__(
      "out %%eax, %%dx;"
  : : "a"(value), "d"(port));
}

uint8_t cpu_inportb(uint32_t port) {
  uint8_t value;
  __asm__ __volatile__(
      "in %%dx, %%al;"
  : "=a"(value) : "d"(port));
  return value;
}

uint16_t cpu_inportw(uint32_t port) {
  uint16_t value;
  __asm__ __volatile__(
      "in %%dx, %%ax;"
  : "=a"(value) : "d"(port));
  return value;
}

uint32_t cpu_inportd(uint32_t port) {
  uint32_t value;
  __asm__ __volatile__(
      "in %%dx, %%eax;"
  : "=a"(value) : "d"(port));
  return value;
}

