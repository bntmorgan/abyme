#include "keyboard.h"
#include "hardware/cpu.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

/**
 * Scancodes to ASCII CODE convertion
 *
 * Theese scancodes are from Eric Alata's Dell Lattitude
 */
char scancodes[DEBUG_SCANCODES_SIZE] = {
  0x00, // Nil
  0x1f, // ESC
  0x31, // 1
  0x32, // 2
  0x33, // 3
  0x34, // 4
  0x35, // 5
  0x36, // 6
  0x37, // 7
  0x38, // 8
  0x39, // 9
  0x30, // 0
  0x29, // °
  0x2b, // +
  0x08, // Backspace
  0x09, // Tab
  0x61, // a
  0x7a, // z
  0x65, // e
  0x72, // r
  0x74, // t
  0x79, // y
  0x75, // u
  0x69, // i
  0x6f, // o
  0x70, // p
  0x5e, // ^
  0x24, // $
  '\r', // Carriage Return
  //0x00, // Carriage Return
  0x00, // Control
  0x71, // q
  0x73, // s
  0x64, // d
  0x66, // f
  0x67, // g
  0x68, // h
  0x6a, // j
  0x6b, // k
  0x6c, // l
  0x6d, // m
  0x00, // ù
  0x00, // Power two
  0x00, // Left shift
  0x2a, // *
  0x77, // w
  0x78, // x
  0x63, // c
  0x76, // v
  0x62, // b
  0x6e, // n
  0x2c, // ,
  0x3b, // ;
  0x3a, // :
  0x21, // !
  0x00, // Right Shift
  0x00, // Nill
  0x00, // Alt
  0x20, // Space
  0x00, // Caps lock
  0x00, // F1
  0x00, // F2
  0x00, // F3
  0x00, // F4
  0x00, // F5
  0x00, // F6
  0x00, // F7
  0x00, // F8
  0x00, // F9
  0x00, // F10
  0x00, // Nil
  0x00, // Nil
  0x00, // Line start
  0x00, // Up arrow
  0x00, // Page up
  0x00, // Nil
  0x00, // Left arrow
  0x00, // Nil
  0x00, // Right arrow
  0x00, // Nil
  0x00, // Line end
  0x00, // Bottom arrow
  0x00, // Page down
  0x00, // insert
  0x7f, // delete
  0x00, // Nil
  0x00, // Nil
  0x00, // Nil
  0x00, // Nil
  0x00, // Nil
  0x00, // Nil
  0x00, // Nil
  0x00, // Mod (Windows)
};

uint8_t keyboard_decode(uint8_t scancode) {
  return scancodes[scancode];
}

uint8_t waitkey() { 
  unsigned char k;
  do {
    k = cpu_inportb(0x60);
  }
  while (k < 128);
  do {
    k = cpu_inportb(0x60);
  }
  while (k > 128);
  return k;
}
