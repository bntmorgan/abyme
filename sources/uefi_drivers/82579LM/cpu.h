#ifndef __CPU_H__
#define __CPU_H__

#include <efi.h>

#include "types.h"

void cpu_outportb(uint32_t port, uint8_t value);

void cpu_outportw(uint32_t port, uint16_t value);

void cpu_outportd(uint32_t port, uint32_t value);

uint8_t cpu_inportb(uint32_t port);

uint16_t cpu_inportw(uint32_t port);

uint32_t cpu_inportd(uint32_t port);

#endif
