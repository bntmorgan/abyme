#ifndef __CPU_H__
#define __CPU_H__

#include <efi.h>

#include "types.h"

inline void cpu_outportb(uint32_t port, uint8_t value);
inline void cpu_outportw(uint32_t port, uint16_t value);
inline void cpu_outportd(uint32_t port, uint32_t value);
inline uint8_t cpu_inportb(uint32_t port);
inline uint16_t cpu_inportw(uint32_t port);
inline uint32_t cpu_inportd(uint32_t port);
inline void cpu_mem_writeb(void *p, uint8_t data);
inline uint8_t cpu_mem_readb(void *p);
inline void cpu_mem_writew(void *p, uint16_t data);
inline uint16_t cpu_mem_readw(void *p);
inline void cpu_mem_writed(void *p, uint32_t data);
inline uint32_t cpu_mem_readd(void *p);
inline void cpu_mem_writeq(void *p, uint64_t data);
inline uint64_t cpu_mem_readq(void *p);

#endif
