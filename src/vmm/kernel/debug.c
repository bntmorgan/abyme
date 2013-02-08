#include "debug.h"
#include "stdio.h"
#include "vmm.h"

uint64_t breakpoints[DEBUG_BREAKPOINTS_SIZE];
int bsize;
int step = 1;
char input[DEBUG_INPUT_SIZE];

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

#define DEBUG_PRINT_USAGE \
  printk("\n"); \
  printk("b : new breakpoint\n"); \
  printk("d : del\n"); \
  printk("p : print breakpoints\n"); \
  printk("v : print the current state\n"); \
  printk("w : print the last state\n"); \
  printk("x : print the changes\n"); \
  printk("c : continue execution\n"); \
  printk("s : step by step\n"); \
  printk("h : help\n"); \

#define DEBUG_PRINT_PROMPT \
  printk("debug$ "); \

#define DEBUG_HANDLE_BREAKPOINT_PRINT \
  printk("\n"); \
  debug_breakpoint_print();

#define DEBUG_HANDLE_BREAKPOINT_ADD \
  printk(" address ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = atoi_hexa(input); \
  debug_breakpoint_add(addr);

#define DEBUG_HANDLE_BREAKPOINT_DEL \
  printk(" number ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t idx = atoi_hexa(input); \
  debug_breakpoint_del(idx);

void debug(int reason) {
  if (reason != EXIT_REASON_MONITOR_TRAP_FLAG) {
    return;
  }
  if (!step) {
    return;
  }
  char c;
  int run = -1;
  while(run) {
    DEBUG_PRINT_PROMPT
    c = getchar();
    switch (c) {
      case 'b':
        DEBUG_HANDLE_BREAKPOINT_ADD
        break; 
      case 'd':
        DEBUG_HANDLE_BREAKPOINT_DEL
        break; 
      case 'p':
        DEBUG_HANDLE_BREAKPOINT_PRINT
        break; 
      case 'v':
        break; 
      case 'w':
        break; 
      case 'x':
        break; 
      case 'c':
        run = 0;
        step = 0;
        break; 
      case 's':
        run = 0;
        step = 1;
        break; 
      case 'h':
      default:
        DEBUG_PRINT_USAGE
    }
    printk("\n");
  }
}

void debug_breakpoint_add(uint64_t address) {
  if (bsize < DEBUG_BREAKPOINTS_SIZE) {
    breakpoints[bsize] = address;
    bsize++;
  }
}

void debug_breakpoint_del(int index) {
  if (bsize > 0 && index >= 1 && index <= bsize) {
    breakpoints[index - 1] = breakpoints[bsize - 1];
    bsize--;
  }
}

void debug_breakpoint_print() {
  int i;
  printk("Breakpoints :\n");
  for (i = 0; i < bsize; ++i) {
    printk("%x : %x\n", i + 1, breakpoints[i]);
  }
}

/** 
 * Wait for keyboard event
 */
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

int pow(int number, unsigned int p) {
  if (p == 0) {
    return 1;
  }
  int n = number;
  unsigned int i;
  for (i = 1; i < p; ++i) {
    n *= number;
  }
  return n;
}

uint64_t atoi_hexa(char *s) {
  unsigned int size = strlen(s), i;
  uint64_t number = 0;
  for (i = 0; i < size; ++i) {
    if (s[i] >= '0' && s[i] <= '9') {
      number += (s[i] - 0x30) * pow(0x10, size - i - 1);
    } else if(s[i] >= 'a' && s[i] <= 'f') {
      number += (s[i] - 'a' + 10) * pow(0x10, size - i - 1);
    }
  }
  return number;
}

unsigned int strlen(char *s) {
  unsigned int size = 0, i;
  for (i = 0; s[i] != '\0'; ++i) {
    ++size;
  }
  return size;
}

char getchar() {
  char c = scancodes[waitkey()];
  printk("%c", c);
  return c;
}

void getstring(char *input, unsigned int size) {
  char c = 0x0;
  unsigned int i = 0;
  while (i < (size - 1)) {
    c = getchar();
    input[i] = c;
    if (c == '\r') {
      break;
    }
    ++i;
  }
  input[i] = '\0';
}
