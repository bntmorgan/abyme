#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "82579LM.h"
#include "cpu.h"

inline uint32_t debug_reg_get(uint32_t reg);
inline void debug_print_reg_stat();
void dump(void *fields, uint32_t fds, uint32_t fdss, uint64_t offset, uint32_t step);

#endif
