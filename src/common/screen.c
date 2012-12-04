#include "arch/cpu_int.h"

#include "screen_int.h"

uint32_t screen_w = 80;
uint32_t screen_h = 25;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

void scr_update_cursor(void) {
  uint16_t location = cursor_y * screen_w + cursor_x;
  cpu_outportb(0x3D4, 14);
  cpu_outportb(0x3D5, location >> 8);
  cpu_outportb(0x3D4, 15);
  cpu_outportb(0x3D5, location);
}

void scr_clear(void) {
  uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = 0; i < screen_w * screen_h; i++) {
    video_memory[i] = 0x0F00;
  }
  cursor_x = 0;
  cursor_y = 0;
  scr_update_cursor();
}

void scr_scroll(void) {
  uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = screen_w; i < screen_w * screen_h; i++) {
    video_memory[i - screen_w] = video_memory[i];
  }
  for (uint32_t i = screen_w * (screen_h - 1); i < screen_w * screen_h; i++) {
    video_memory[i] = 0x0F00;
  }
  cursor_y--;
}

void scr_print(uint8_t value) {
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
    scr_scroll();
  }
  scr_update_cursor();
}
