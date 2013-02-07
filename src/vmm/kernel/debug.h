#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"

#define DEBUG_BREAKPOINTS_SIZE 0xff
#define DEBUG_SCANCODES_SIZE 0xff

uint8_t waitkey();
char getchar();

#endif
