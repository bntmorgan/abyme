#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "vmm.h"
#include "hardware/msr.h"
#include "hardware/cpu.h"

/**
 * Unshared prototypes
 */
void dump_vmcs_fields(uint64_t offset, uint32_t fs, uint32_t count, uint32_t step);
uint64_t debug_shortcut(char *s);
void debug_alter_vmcs(uint32_t field, uint32_t bit, uint8_t on);

struct breakpoint breakpoints[DEBUG_BREAKPOINTS_SIZE];
int bsize;
int step = 0;
char input[DEBUG_INPUT_SIZE];


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
      printk(#field " = 0x%016x\n", state->field); \
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
      printk(#field ": 0x%016x -> 0x%016x\n", state_a->field, state_b->field); \
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
  printk("z : print the last state\n"); \
  printk("x : print the changes\n"); \
  printk("i : print current instruction (mem[rip])\n"); \
  printk("m : dump memory\n"); \
  printk("c : continue execution\n"); \
  printk("s : step by step\n"); \
  printk("h : help\n"); \
  printk("r : read mem\n"); \
  printk("w : write mem\n"); \
  printk("y : vmcs dump\n"); \
  printk("o : set I/O\n"); \
  printk("n : unset I/O\n"); \
}

#define DEBUG_CURRENT_STATE_INDEX (guest_states_index % 2)

#define DEBUG_PREVIOUS_STATE_INDEX ((guest_states_index + 1) % 2)

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
  printk("\naddress ? (shortcuts : rip, rsp, rbp, ...)"); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = debug_shortcut((char*)input); \
  if (addr == (uint64_t)-1) { \
    addr = atoi_hexa(input); \
  } \
  printk("\n %X size ? ", addr); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t size = atoi_hexa(input); \
  if (size == 0) { \
    size = 0x16; \
  } \
  dump(addr, size); \
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
  debug_print_guest_state_diff(guest_states + DEBUG_PREVIOUS_STATE_INDEX, guest_states + DEBUG_CURRENT_STATE_INDEX); \
}

#define DEBUG_HANDLE_SET_IO { \
  printk("\nsetting inconditional I/O ? "); \
  uint32_t procbased_ctls = cpu_vmread(CPU_BASED_VM_EXEC_CONTROL); \
  procbased_ctls |= (uint32_t)UNCOND_IO_EXITING; \
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls); \
}

#define DEBUG_HANDLE_UNSET_IO { \
  printk("\nunsetting inconditional I/O ? "); \
  uint32_t procbased_ctls = cpu_vmread(CPU_BASED_VM_EXEC_CONTROL); \
  procbased_ctls &= ~((uint32_t)UNCOND_IO_EXITING); \
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls); \
}

#define DEBUG_HANDLE_WRITE_MEM { \
  printk("\naddress ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = atoi_hexa(input); \
  printk("\nvalue ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint8_t value = (uint8_t)atoi_hexa(input); \
  printk("Writing %x in %x\n", value, addr); \
  *((uint8_t *)addr) = value; \
}

#define DEBUG_HANDLE_READ_MEM { \
  printk("\naddress ? "); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint64_t addr = atoi_hexa(input); \
  printk("%02x\n", *((uint8_t *)addr)); \
}

#define DEBUG_HANDLE_DUMP_VMCS { \
  printk("\nFields ? \n"); \
  printk("0 : 16 bits control fields\n"); \
  printk("1 : 16 bits guest state fields\n"); \
  printk("2 : 16 bits host state fields\n"); \
  printk("3 : 64 bits control fields\n"); \
  printk("4 : 64 bits read only data fields\n"); \
  printk("5 : 64 bits guest state fields\n"); \
  printk("6 : 64 bits host state fields\n"); \
  printk("7 : 32 bits control fields\n"); \
  printk("8 : 32 bits read only data fields\n"); \
  printk("9 : 32 bits guest state fields\n"); \
  printk("a : 32 bits host state fields\n"); \
  printk("b : NW bits control fields\n"); \
  printk("c : NW bits read only data fields\n"); \
  printk("d : NW bits guest state fields\n"); \
  printk("e : NW bits host state fields\n"); \
  getstring(input, DEBUG_INPUT_SIZE); \
  printk("\n"); \
  uint8_t field = atoi_hexa(input); \
  printk("Field %d\n", field); \
  switch (field) { \
    case 0: \
      dump_vmcs_fields(0, 2, 2, 2); \
      break; \
    case 1: \
      dump_vmcs_fields(0x800, 2, 9, 2); \
      break; \
    case 2: \
      dump_vmcs_fields(0xc00, 2, 7, 2); \
      break; \
    case 3: \
      dump_vmcs_fields(0x2000, 4, 7 * 2, 1); \
      dump_vmcs_fields(0x2010, 4, 11 * 2, 1); \
      break; \
    case 4: \
      dump_vmcs_fields(0x2400, 4, 1 * 2, 1); \
      break; \
    case 5: \
      dump_vmcs_fields(0x2800, 4, 9 * 2, 1); \
      break; \
    case 6: \
      dump_vmcs_fields(0x2c00, 4, 3 * 2, 1); \
      break; \
    case 7: \
      dump_vmcs_fields(0x4000, 4, 18, 2); \
      break; \
    case 8: \
      dump_vmcs_fields(0x4400, 4, 8, 2); \
      break; \
    case 9: \
      dump_vmcs_fields(0x4800, 4, 22, 2); \
      dump_vmcs_fields(0x482e, 4, 1, 2); \
      break; \
    case 10: \
      dump_vmcs_fields(0x4c00, 4, 1, 2); \
      break; \
    case 11: \
      dump_vmcs_fields(0x6000, 4, 8, 2); \
      break; \
    case 12: \
      dump_vmcs_fields(0x6400, 4, 6, 2); \
      break; \
    case 13: \
      dump_vmcs_fields(0x6800, 4, 20, 2); \
      break; \
    case 14: \
      dump_vmcs_fields(0x6c00, 4, 12, 2); \
      break; \
  } \
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
    c = (char)getchar();
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
      case 'z':
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
        if (step) {
          debug_alter_vmcs(CPU_BASED_VM_EXEC_CONTROL, MONITOR_TRAP_FLAG, 0); \
          step = 0;
        }
        break; 
      case 's':
        run = 0;
        if (!step) {
          step = 1;
          debug_alter_vmcs(CPU_BASED_VM_EXEC_CONTROL, MONITOR_TRAP_FLAG, 1); \
        }
        break; 
      case 'o':
        DEBUG_HANDLE_SET_IO
        break;
      case 'n':
        DEBUG_HANDLE_UNSET_IO
        break; 
      case 'w':
        DEBUG_HANDLE_WRITE_MEM
        break; 
      case 'r':
        DEBUG_HANDLE_READ_MEM
        break;
      case 'y':
        DEBUG_HANDLE_DUMP_VMCS
        break;
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
  dump(rip, length);
}

void debug_breakpoint_add(uint64_t address) {
  if (bsize < DEBUG_BREAKPOINTS_SIZE) {
    // Backup the code
    //INFO("Save %02x %02x %02x\n", *((uint8_t *)address + 0), *((uint8_t *)address + 1), *((uint8_t *)address + 2));
    breakpoints[bsize].code[0] = *((uint8_t *)address + 0);
    breakpoints[bsize].code[1] = *((uint8_t *)address + 1);
    breakpoints[bsize].code[2] = *((uint8_t *)address + 2);
    // Put the vmcall
    *((uint8_t *)address + 0) = 0x0f; // vmcall
    *((uint8_t *)address + 1) = 0x01;
    *((uint8_t *)address + 2) = 0xc1;
    //INFO("Set vmcall %02x %02x %02x\n", *((uint8_t *)address + 0), *((uint8_t *)address + 1), *((uint8_t *)address + 2));
    if (*((uint8_t *)address + 0) != 0x0f ||
        *((uint8_t *)address + 1) != 0x01 ||
        *((uint8_t *)address + 2) != 0xc1 ) {
      INFO("Warning : the memory seems to be protected : breakpoint wont be effective\n");
    }

    breakpoints[bsize].address = address;
    bsize++;
  }
}

void debug_breakpoint_del(int index) {
  if (index >= 0 && index < bsize) {
    // Restore the code
    //INFO("Before restore %02x %02x %02x\n", *((uint8_t *)breakpoints[index].address + 0), *((uint8_t *)breakpoints[index].address + 1), *((uint8_t *)breakpoints[index].address + 2));
    *((uint8_t *)breakpoints[index].address + 0) = breakpoints[index].code[0];
    *((uint8_t *)breakpoints[index].address + 1) = breakpoints[index].code[1];
    *((uint8_t *)breakpoints[index].address + 2) = breakpoints[index].code[2];
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
    printk("%06x : %x\n", i, breakpoints[i].address);
    printk("       : %x\n", *((uint32_t *)breakpoints[i].code)); // XXX moisi
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

void getstring(char *input, unsigned int size) {
  char c = 0x0;
  unsigned int i = 0;
  while (i < (size - 1)) {
    c = (char)getchar();
    input[i] = c;
    if (c == '\r') {
      break;
    }
    ++i;
  }
  input[i] = '\0';
}

/**
 * Fields is the address of a 16 bits fields structure
 */
#define DUMP(fields, fds, fdss, offset, step) { \
  uint32_t i, j; \
  uint32_t cycles = fdss / fds; \
  for (i = 0; i < cycles; i++) { \
    if (i % 4 == 0) { \
      printk("%08x: ", i * step + offset); \
    } \
    for (j = 0; j < fds; j++) { \
      printk("%02x", *((uint8_t*)fields + i * fds + (fds - j - 1))); \
    } \
    printk(" "); \
    if (i % 4 == 3) { \
      printk("\n"); \
    } \
  }\
  if (i % 4 != 3) { \
    printk("\n"); \
  } \
}

#define MEMCPY(dst, src, size) { \
  uint32_t ii; \
  for (ii = 0; ii < size; ii++) { \
    *((uint8_t*)dst + ii) = *((uint8_t*)src + ii); \
  } \
}

void read_vmcs_fields(uint32_t offset, uint32_t fs, uint32_t count, uint32_t step, uint8_t *fields) {
  uint32_t i, field, f = offset;
  for (i = 0; i < count; i++) {
    field = cpu_vmread(f);
    MEMCPY(fields + (fs * i), (uint8_t*)&field, fs);
    f += step;
  }
}

void dump_vmcs_fields(uint64_t offset, uint32_t fs, uint32_t count, uint32_t step) {
  uint8_t fields[4096];
  read_vmcs_fields(offset, fs, count, step, (uint8_t *)&fields[0]);
  DUMP(&fields, fs, count * fs, offset, step);
}

uint8_t debug_strcmp(char *a, char *b) {
  if (*a == '\0') {
    return -1;
  }
  if (*b == '\0') {
    return 1;
  }
  while (*a != '\0' && *b != '\0') {
    if (*a < *b) {
      return -1;
    }
    if (*a > *b) {
      return 1;
    }
    a++;
    b++;
  }
  if (*a == '\0' && *b != '\0') {
    return -1;
  }
  if (*b == '\0' && *a != '\0') {
    return 1;
  }
  return 0;
}

uint64_t debug_shortcut(char *s) {
  // registres
  if (debug_strcmp(s, "rsp") == 0) {
    return (*(guest_states + DEBUG_CURRENT_STATE_INDEX)).rsp;
  } else if(debug_strcmp(s, "rip") == 0) {
    return (*(guest_states + DEBUG_CURRENT_STATE_INDEX)).rip;
  } else if(debug_strcmp(s, "rbp") == 0) {
    return (*(guest_states + DEBUG_CURRENT_STATE_INDEX)).rbp;
  }
  // No shortcut found
  return -1;
}

void debug_alter_vmcs(uint32_t field, uint32_t bit, uint8_t on) {
  INFO("\n");
  uint32_t f = cpu_vmread(field);
  INFO("Before %x\n", f);
  if (on) {
    f |= bit;
  } else {
    f &= ~(bit);
  }
  INFO("After %x\n", f);
  cpu_vmwrite(field, f);
}
