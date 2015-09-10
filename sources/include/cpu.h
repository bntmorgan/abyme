#ifndef __CPU_H__
#define __CPU_H__

#include <efi.h>
#include "types.h"

struct idt_ptr {
  uint64_t limit:16;
  uint64_t base:64;
} __attribute__((packed));

union rflags {
  struct {
    uint64_t cf:1; // 0
    uint64_t o0:1; // 1
    uint64_t pf:1; // 2
    uint64_t z0:1; // 3
    uint64_t af:1; // 4
    uint64_t z1:1; // 5
    uint64_t zf:1; // 6
    uint64_t sf:1; // 7
    uint64_t tf:1; // 8
    uint64_t IF:1; // 9
    uint64_t df:1; // 10
    uint64_t of:1; // 11
    uint64_t iopl:2; // 12 - 13
    uint64_t nt:1; // 14
    uint64_t z2:1; // 15
    uint64_t rf:1; // 16
    uint64_t vm:1; // 17
    uint64_t ac:1; // 18
    uint64_t vif:1; // 19
    uint64_t vip:1; // 20
    uint64_t id:1; // 21
    uint64_t r0:42; // 22
  };
  uint64_t raw;
};

union cr0 {
  struct {
    uint64_t pe:1;
    uint64_t mp:1;
    uint64_t em:1;
    uint64_t ts:1;
    uint64_t et:1;
    uint64_t ne:1;
    uint64_t r0:10;
    uint64_t wp:1;
    uint64_t r1:1;
    uint64_t am:1;
    uint64_t r2:10;
    uint64_t nw:1;
    uint64_t cd:1;
    uint64_t pg:1;
    uint64_t r3:32;
  };
  uint64_t raw;
};

union cr3 {
  struct {
    uint64_t r0:3;
    uint64_t pwt:1;
    uint64_t pcd:1;
    uint64_t r1:7;
    uint64_t page_directory_base:20;
  };
  uint64_t raw;
};

union cr4 {
  struct {
    uint64_t vme:1;
    uint64_t pvi:1;
    uint64_t tsd:1;
    uint64_t de:1;
    uint64_t pse:1;
    uint64_t pae:1;
    uint64_t mce:1;
    uint64_t pge:1;
    uint64_t pce:1;
    uint64_t osfxsr:1;
    uint64_t osxmmexcpt:1;
    uint64_t r0:2;
    uint64_t vmxe:1;
    uint64_t smxe:1;
    uint64_t r1:1;
    uint64_t fsgsbase:1;
    uint64_t pcide:1;
    uint64_t osxsave:1;
    uint64_t r2:1;
    uint64_t smep:1;
    uint64_t r3:11;
  };
  uint64_t raw;
};

void cpu_read_gdt(uint8_t *gdt_ptr);
void cpu_read_idt(struct idt_ptr *idt_ptr);
void cpu_write_idt(struct idt_ptr *idt_ptr);
uint64_t cpu_read_cr0(void);
uint64_t cpu_read_cr2(void);
uint64_t cpu_read_cr3(void);
uint64_t cpu_read_cr4(void);
uint64_t cpu_read_cs(void);
uint64_t cpu_read_ss(void);
uint64_t cpu_read_ds(void);
uint64_t cpu_read_es(void);
uint64_t cpu_read_fs(void);
uint64_t cpu_read_gs(void);
uint64_t cpu_read_tr(void);
uint64_t cpu_read_ldtr(void);
uint64_t cpu_read_dr7(void);
void cpu_write_cr0(uint64_t reg);
void cpu_write_cr2(uint64_t reg);
void cpu_write_cr3(uint64_t reg);
void cpu_write_cr4(uint64_t reg);
void cpu_enable_ne(void);
void cpu_stop(void);
uint64_t cpu_read_flags(void);
uint64_t cpu_rdtsc(void);

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

///* BOCHS magic breakpoint */
////#define BREAKPOINT() __asm__ __volatile__("xchg %bx, %bx")
//void cpu_outportb(uint32_t port, uint8_t value);
//uint8_t cpu_inportb(uint32_t port);
//void cpu_write_cr3(uint64_t reg);
//uint32_t cpu_get_seg_desc_base(uint64_t gdt_base, uint16_t seg_sel);
//void cpu_vmresume(void);
//uint8_t cpu_is_paging_enabled(void);
//uint8_t cpu_is_protected_mode_enabled(void);
//void cpu_enable_pae(void);
//void cpu_enable_paging(void);
//void cpu_enable_long_mode(void);
//uint8_t cpu_is_ept_supported(void);
//uint8_t cpu_is_unrestricted_guest_supported(void);
//uint64_t cpu_read_rsp(void);

#endif
