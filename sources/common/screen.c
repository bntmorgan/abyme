#include "screen.h"

#include "hardware/cpu.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

uint32_t screen_w = 80;
uint32_t screen_h = 25;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

void screen_update_cursor(void) {
  uint16_t location = cursor_y * screen_w + cursor_x;
  cpu_outportb(0x3D4, 14);
  cpu_outportb(0x3D5, location >> 8);
  cpu_outportb(0x3D4, 15);
  cpu_outportb(0x3D5, location);
}

void screen_clear(void) {
  uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = 0; i < screen_w * screen_h; i++) {
    video_memory[i] = 0x0F00;
  }
  cursor_x = 0;
  cursor_y = 0;
  screen_update_cursor();
}

void screen_scroll(void) {
  uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = screen_w; i < screen_w * screen_h; i++) {
    video_memory[i - screen_w] = video_memory[i];
  }
  for (uint32_t i = screen_w * (screen_h - 1); i < screen_w * screen_h; i++) {
    video_memory[i] = 0x0F00;
  }
  cursor_y--;
}

void screen_print(uint8_t value) {
  uint8_t *video_memory = (uint8_t *) 0xb8000;
  if (value == '\n') {
    cursor_y++;
    cursor_x = 0;
  } else {
    video_memory[(cursor_y * screen_w + cursor_x) * 2] = value;
    cursor_x++;
  }
  if (cursor_x == screen_w) {
    cursor_x = 0;
    cursor_y++;
  }
  if (cursor_y == screen_h) {
    screen_scroll();
  }
  screen_update_cursor();
}
