#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"

#define DEBUG_BREAKPOINTS_SIZE 0xff
#define DEBUG_SCANCODES_SIZE 0xff
#define DEBUG_INPUT_SIZE 0x20

uint8_t waitkey();
char getchar();
void debug_breakpoint_add(uint64_t address);
void debug_breakpoint_del(int index);
void debug_breakpoint_print();
void debug(int reason);
void getstring(char *input, unsigned int size);
unsigned int strlen(char *c);
uint64_t atoi_hexa(char *s);

#endif
