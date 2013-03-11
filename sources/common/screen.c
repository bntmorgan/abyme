#include "screen.h"

#include "hardware/cpu.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

uint32_t screen_w = 80;
uint32_t screen_h = 25;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

#ifdef _CODE16GCC_
static inline void set_video_mem(uint16_t index, uint16_t value) {
  __asm__ __volatile__(
      "mov %%ax, %%ds ;"
      "mov %%ebx, (%%edx, %%ecx, 2) ;"
      "xor %%ax, %%ax ;"
      "mov %%ax, %%ds ;"
  : : "a"(0xb800), "b"(value), "c"(index), "d"(0x00));
}
#else
static inline void set_video_mem(uint16_t index, uint16_t value) {
  *(((uint16_t*)0xb8000) + (index)) = (value);
}
#endif

#ifdef _CODE16GCC_
static inline uint16_t get_video_mem(uint16_t index) {
  uint16_t val;
  __asm__ __volatile__(
      "push %%ds ;"
      "mov %%ax, %%ds ;"
      "mov (%%ecx, %%ebx, 2), %%edx;"
      "pop %%ds ;"
  : "=d"(val) : "a"(0xb800), "b"(index), "c"(0x00));
  return val;
}
#else
static inline uint16_t get_video_mem(uint16_t index) {
  return *(((uint16_t*)0xb8000) + (index));
}
#endif

void screen_update_cursor(void) {
  uint16_t location = cursor_y * screen_w + cursor_x;
  cpu_outportb(0x3D4, 14);
  cpu_outportb(0x3D5, location >> 8);
  cpu_outportb(0x3D4, 15);
  cpu_outportb(0x3D5, location);
}

void screen_clear(void) {
  //uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = 0; i < screen_w * screen_h; i++) {
    //video_memory[i] = 0x0F00;
    set_video_mem(i, 0x0f00);
  }
  cursor_x = 0;
  cursor_y = 0;
  screen_update_cursor();
}

void screen_scroll(void) {
  //uint16_t *video_memory = (uint16_t *) 0xb8000;
  for (uint32_t i = screen_w; i < screen_w * screen_h; i++) {
    //video_memory[i - screen_w] = video_memory[i];
    set_video_mem(i - screen_w, get_video_mem(i));
  }
  for (uint32_t i = screen_w * (screen_h - 1); i < screen_w * screen_h; i++) {
    //video_memory[i] = 0x0F00;
    set_video_mem(i, 0x0f00);
  }
  cursor_y--;
}

void screen_print(uint8_t value) {
  //uint8_t *video_memory = (uint8_t *) 0xb8000;
  //uint16_t *video_memory = (uint16_t *) 0xb8000;
  if (value == '\n') {
    cursor_y++;
    cursor_x = 0;
  } else {
    //video_memory[(cursor_y * screen_w + cursor_x)] = (((uint16_t)value & 0x00ff) | 0x0f00);
    //*(((uint16_t*)0xb8000) + (cursor_y * screen_w + cursor_x)) = (((uint16_t)value & 0x00ff) | 0x0f00);
    set_video_mem((cursor_y * screen_w + cursor_x), (((uint16_t)value & 0x00ff) | 0x0f00));
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
