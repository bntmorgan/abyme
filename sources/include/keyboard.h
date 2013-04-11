#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <efi.h>
#include "types.h"

#define DEBUG_SCANCODES_SIZE 0xff

/** 
 * Wait for keyboard event
 */
uint8_t waitkey();
uint8_t keyboard_decode(uint8_t scancode);

#endif
