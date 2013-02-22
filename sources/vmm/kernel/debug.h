#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"
#include "hardware/cpu.h"

#define DEBUG_BREAKPOINTS_SIZE 0xff
#define DEBUG_SCANCODES_SIZE 0xff
#define DEBUG_INPUT_SIZE 0x20

#define DEBUG_VMM_ENTRY_POINT 0x700

uint8_t waitkey();
char getchar();
void debug_breakpoint_add(uint64_t address);
void debug_breakpoint_del(int index);
void debug_breakpoint_print();
int debug(uint32_t reason);
void getstring(char *input, unsigned int size);
unsigned int strlen(char *c);
uint64_t atoi_hexa(char *s);
int debug_breakpoint_break(uint64_t rip);
void debug_instruction_print(uint64_t rip, uint32_t length);
void debug_dump(uint64_t addr, uint64_t size);

#define DEBUG_GUEST_STATE \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, cr0, GUEST_CR0) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, cr3, GUEST_CR3) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, cr4, GUEST_CR4) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, cs_base, GUEST_CS_BASE) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, ds_base, GUEST_DS_BASE) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, es_base, GUEST_ES_BASE) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_VMCS, ss_base, GUEST_SS_BASE) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rip) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rsp) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rbp) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rax) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rbx) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rcx) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rdx) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rsi) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, rdi) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r8) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r9) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r10) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r11) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r12) \
  DEBUG_GUEST_STATE_FIELD(DEBUG_FIELD_REG, r13)

#define DEBUG_GUEST_STATE_FIELD_TOKENPASTE(x, y) x ## _ ## y
#define DEBUG_GUEST_STATE_FIELD_TOKENPASTE2(x, y) DEBUG_GUEST_STATE_FIELD_TOKENPASTE(x, y)
#define DEBUG_GUEST_STATE_FIELD(type, ...) DEBUG_GUEST_STATE_FIELD_TOKENPASTE2(DEBUG_GUEST_STATE_FIELD, type)(__VA_ARGS__)

#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG(field) uint64_t field;
#define DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS(field, vmcs_field) uint64_t field;
//#define DEBUG_GUEST_STATE_FIELD(type, field, ...) uint64_t field;
struct guest_state {
  DEBUG_GUEST_STATE
};
//#undef DEBUG_GUEST_STATE_FIELD
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_REG
#undef DEBUG_GUEST_STATE_FIELD_DEBUG_FIELD_VMCS

#define NB_GUEST_STATES 2
extern struct guest_state guest_states[NB_GUEST_STATES];
extern uint32_t guest_states_index;
void debug_print_guest_state(struct guest_state *state, uint32_t field_index_from, uint32_t field_index_to);
void debug_save_guest_state(struct guest_state *state, gpr64_t *guest_gpr);
void debug_install();

struct breakpoint {
  uint64_t address;
  uint8_t code[3];
};

#endif
