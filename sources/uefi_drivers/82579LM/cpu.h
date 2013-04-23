#ifndef __CPU_H__
#define __CPU_H__

#include <efi.h>

#include "types.h"

static inline void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__(
      "out %%al, %%dx;"
      : : "a"(value), "d"(port));
}

static inline void cpu_outportw(uint32_t port, uint16_t value) {
  __asm__ __volatile__(
      "out %%ax, %%dx;"
      : : "a"(value), "d"(port));
}

static inline void cpu_outportd(uint32_t port, uint32_t value) {
  __asm__ __volatile__(
      "out %%eax, %%dx;"
      : : "a"(value), "d"(port));
}

static inline uint8_t cpu_inportb(uint32_t port) {
  uint8_t value;
  __asm__ __volatile__(
      "in %%dx, %%al;"
      : "=a"(value) : "d"(port));
  return value;
}

static inline uint16_t cpu_inportw(uint32_t port) {
  uint16_t value;
  __asm__ __volatile__(
      "in %%dx, %%ax;"
      : "=a"(value) : "d"(port));
  return value;
}

static inline uint32_t cpu_inportd(uint32_t port) {
  uint32_t value;
  __asm__ __volatile__(
      "in %%dx, %%eax;"
      : "=a"(value) : "d"(port));
  return value;
}

static inline void cpu_mem_writeb(void *p, uint8_t data) {
  *(volatile uint8_t *)(p) = data;
}

static inline uint8_t cpu_mem_readb(void *p) {
  return *(volatile uint8_t *)(p);
}

static inline void cpu_mem_writew(void *p, uint16_t data) {
  *(volatile uint16_t *)(p) = data;
}

static inline uint16_t cpu_mem_readw(void *p) {
  return *(volatile uint16_t *)(p);
}

static inline void cpu_mem_writed(void *p, uint32_t data) {
  *(volatile uint32_t *)(p) = data;
}

static inline uint32_t cpu_mem_readd(void *p) {
  return *(volatile uint32_t *)(p);
}

static inline void cpu_mem_writeq(void *p, uint64_t data) {
  *(volatile uint64_t *)(p) = data;
}

static inline uint64_t cpu_mem_readq(void *p) {
  return *(volatile uint64_t *)(p);
}

#endif
