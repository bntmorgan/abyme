#include "debug.h"
#include "stdio.h"
#include "vmm.h"

struct breakpoint breakpoints[DEBUG_BREAKPOINTS_SIZE];
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

//#define GUEST_CREG     0
//#define GUEST_SEG_BASE 1
//#define GUEST_REG      2
uint32_t guest_states_index = 0;
// TODO: fill with 0!!!
struct guest_state guest_states[NB_GUEST_STATES];

#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG(field) state-> field = guest_gpr-> field;
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS(field, vmcs_field) state-> field = cpu_vmread(vmcs_field);
void debug_save_guest_state(struct guest_state *state, gpr64_t *guest_gpr) {
  DEBUG_GUEST_STATE
}
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS

#define DEBUG_GUEST_STATE_FIELD_PRINT(field) \
  do {\
    if (field_index_from <= i && i <= field_index_to) {\
      INFO(#field " = 0x%016x\n", state->field); \
    }\
    i++;\
  } while (0);
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG(field) DEBUG_GUEST_STATE_FIELD_PRINT(field)
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS(field, vmcs_field) DEBUG_GUEST_STATE_FIELD_PRINT(field)
void debug_print_guest_state(struct guest_state *state, uint32_t field_index_from, uint32_t field_index_to) {
  uint32_t i = 0;
  DEBUG_GUEST_STATE
}
#undef DEBUG_GUEST_STATE_FIELD_PRINT_DIFF
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS

#define DEBUG_GUEST_STATE_FIELD_PRINT_DIFF(field) \
  do {\
    if (state_a->field != state_b->field) { \
      INFO(#field ": 0x%016x -> 0x%016x\n", state_a->field, state_b->field); \
    }\
  } while (0);
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG(field) DEBUG_GUEST_STATE_FIELD_PRINT_DIFF(field)
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS(field, vmcs_field) DEBUG_GUEST_STATE_FIELD_PRINT_DIFF(field)
void debug_print_guest_state_diff(struct guest_state *state_a, struct guest_state *state_b) {
  DEBUG_GUEST_STATE
}

#define DEBUG_PRINT_USAGE {\
  printk("\n"); \
  printk("b : new breakpoint\n"); \
  printk("d : del\n"); \
  printk("p : print breakpoints\n"); \
  printk("v : print the current state\n"); \
  printk("w : print the last state\n"); \
  printk("x : print the changes\n"); \
  printk("i : print current instruction (mem[rip])\n"); \
  printk("m : dump memory\n"); \
  printk("c : continue execution\n"); \
  printk("s : step by step\n"); \
  printk("h : help\n"); \
  printk("r : read mem\n"); \
  printk("w : write mem\n"); \
  printk("o : set I/O\n"); \
  printk("n : unset I/O\n"); \
}

#define DEBUG_CURRENT_STATE_INDEX (guest_states_index % 2)

#define DEBUG_PREVIOUS_STATE_INDEX (guest_states_index % 2 + 1)

#define DEBUG_PRINT_PROMPT {\
  printk("debug@%x[%x]$", guest_states[DEBUG_CURRENT_STATE_INDEX].rip, reason); \
}

#define DEBUG_HANDLE_BREAKPOINT_PRINT {\
  printk("\n"); \
  debug_breakpoint_print(); \
}

#define DEBUG_HANDLE_INSTRUCTION_PRINT {\
  printk("\n"); \
  debug_instruction_print(guest_states[DEBUG_CURRENT_STATE_INDEX].rip, 16); \
}

#define DEBUG_HANDLE_BREAKPOINT_ADD {\
  printk(" address ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = atoi_hexa(input); \
  debug_breakpoint_add(addr); \
}

#define DEBUG_HANDLE_DUMP {\
  printk("\naddress ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = atoi_hexa(input); \
  printk("\nsize ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t size = atoi_hexa(input); \
  debug_dump(addr, size); \
}

#define DEBUG_HANDLE_BREAKPOINT_DEL {\
  printk(" number ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t idx = atoi_hexa(input); \
  debug_breakpoint_del(idx); \
}

#define DEBUG_HANDLE_STATE_PRINT {\
  printk("\n"); \
  debug_print_guest_state(guest_states + DEBUG_CURRENT_STATE_INDEX, 0, 21); \
}

#define DEBUG_HANDLE_LAST_STATE_PRINT {\
  printk("\n"); \
  debug_print_guest_state(guest_states + DEBUG_PREVIOUS_STATE_INDEX, 0, 21); \
}

#define DEBUG_HANDLE_STATE_DIFF {\
  printk("\n"); \
  debug_print_guest_state_diff(guest_states, guest_states + 1); \
}

int debug(uint32_t reason, int force) {
  int bp = debug_breakpoint_break(guest_states[DEBUG_CURRENT_STATE_INDEX].rip);
  if (!step && bp == -1 && !force) {
    return 0;
  }
  if (force == 0) {
    debug_breakpoint_del(bp);
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
        DEBUG_HANDLE_STATE_PRINT
        break; 
      case 'l':
        DEBUG_HANDLE_LAST_STATE_PRINT
        break; 
      case 'x':
        DEBUG_HANDLE_STATE_DIFF
        break; 
      case 'i':
        DEBUG_HANDLE_INSTRUCTION_PRINT
        break; 
      case 'm':
        DEBUG_HANDLE_DUMP
        break; 
      case 'c':
        run = 0;
        step = 0;
        break; 
      case 's':
        run = 0;
        step = 1;
        break; 
      case 'o': {
        printk("\nsetting inconditional I/O ? ");
        uint32_t procbased_ctls = cpu_vmread(CPU_BASED_VM_EXEC_CONTROL);
        procbased_ctls |= (uint32_t)UNCOND_IO_EXITING; 
        cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);
        break; 
                }
      case 'n': {
        printk("\nunsetting inconditional I/O ? ");
        uint32_t procbased_ctls = cpu_vmread(CPU_BASED_VM_EXEC_CONTROL);
        procbased_ctls &= ~((uint32_t)UNCOND_IO_EXITING); 
        cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);
        break; 
                }
      case 'w': {
        printk("\naddress ? "); \
        getstring(input, DEBUG_INPUT_SIZE); \
        printk("\n"); \
        uint64_t addr = atoi_hexa(input); \
        printk("\nvalue ? "); \
        getstring(input, DEBUG_INPUT_SIZE); \
        printk("\n"); \
        uint8_t value = (uint8_t)atoi_hexa(input); \
        printk("Writing %x in %x\n", value, addr);
        *((uint8_t *)addr) = value;
        break; 
                }
      case 'r': {
        printk("\naddress ? "); \
        getstring(input, DEBUG_INPUT_SIZE); \
        printk("\n"); \
        uint64_t addr = atoi_hexa(input); \
        printk("%02x\n", *((uint8_t *)addr));
        break; 
                }
      case 'h':
      default:
        DEBUG_PRINT_USAGE
    }
    printk("\n");
  }
  return 1;
}

void debug_install() {
  debug_breakpoint_add(DEBUG_VMM_ENTRY_POINT);
}

void debug_instruction_print(uint64_t rip, uint32_t length) {
  printk("Last instruction\n");
  debug_dump(rip, length);
}

#define DEBUG_DUMP_COLUMNS 6

void debug_dump(uint64_t addr, uint64_t size) {
  uint32_t i = 0, j;
  while (i < size) {
    printk("%016x ", addr + i);
    for (j = 0; j < DEBUG_DUMP_COLUMNS; j++) {
      printk("%08x", *((uint32_t *)(addr + i)));
      if (i != DEBUG_DUMP_COLUMNS - 1) {
        printk(" ");
      }
      i += 4;
      if (i >= size) {
        break;
      }
    }
    printk("\n");
  }
}

void debug_breakpoint_add(uint64_t address) {
  if (bsize < DEBUG_BREAKPOINTS_SIZE) {
    // Backup the code
    INFO("Save %02x %02x %02x\n", *((uint8_t *)address + 0), *((uint8_t *)address + 1), *((uint8_t *)address + 2));
    breakpoints[bsize].code[0] = *((uint8_t *)address + 0);
    breakpoints[bsize].code[1] = *((uint8_t *)address + 1);
    breakpoints[bsize].code[2] = *((uint8_t *)address + 2);
    // Put the vmcall
    *((uint8_t *)address + 0) = 0x0f; // vmcall
    *((uint8_t *)address + 1) = 0x01;
    *((uint8_t *)address + 2) = 0xc1;
    INFO("Set vmcall %02x %02x %02x\n", *((uint8_t *)address + 0), *((uint8_t *)address + 1), *((uint8_t *)address + 2));
    breakpoints[bsize].address = address;
    bsize++;
  }
}

void debug_breakpoint_del(int index) {
  if (index >= 0 && index < bsize) {
    // Restore the code
    INFO("Before restore %02x %02x %02x\n", *((uint8_t *)breakpoints[index].address + 0), *((uint8_t *)breakpoints[index].address + 1), *((uint8_t *)breakpoints[index].address + 2));
    *((uint8_t *)breakpoints[index].address + 0) = breakpoints[index].code[0];
    *((uint8_t *)breakpoints[index].address + 1) = breakpoints[index].code[1];
    *((uint8_t *)breakpoints[index].address + 2) = breakpoints[index].code[2];
    INFO("After restore %02x %02x %02x\n", *((uint8_t *)breakpoints[index].address + 0), *((uint8_t *)breakpoints[index].address + 1), *((uint8_t *)breakpoints[index].address + 2));
    // Exchange the breakpoint
    if (bsize > 1) {
      breakpoints[index].address = breakpoints[bsize - 1].address;
      breakpoints[index].code[0] = breakpoints[bsize - 1].code[0];
      breakpoints[index].code[1] = breakpoints[bsize - 1].code[1];
      breakpoints[index].code[2] = breakpoints[bsize - 1].code[2];
    }
    bsize--;
  }
}

void debug_breakpoint_print() {
  int i;
  printk("Breakpoints :\n");
  for (i = 0; i < bsize; ++i) {
    printk("%x : %x\n", i, breakpoints[i].address);
    printk("   : %x\n", *((uint32_t *)breakpoints[i].code)); // XXX moisi
  }
}

int debug_breakpoint_break(uint64_t rip) {
  int i;
  for (i = 0; i < bsize; ++i) {
    if (rip == breakpoints[i].address) {
      return i;
    }
  }
  return -1;
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
