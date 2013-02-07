#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"

#define DEBUG_BREAKPOINTS_SIZE 0xff
#define DEBUG_SCANCODES_SIZE 0xff

uint8_t waitkey();
char getchar();
void debug_breakpoint_add(uint64_t address);
void debug_breakpoint_del(int index);

#endif
