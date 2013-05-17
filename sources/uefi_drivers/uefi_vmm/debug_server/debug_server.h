#ifndef __DEBUG_SERVER_H__
#define __DEBUG_SERVER_H__
#include "efi/efi_82579LM.h"
#include "vmm.h"

void debug_server_init();
void debug_server_send(void *buf, uint32_t len);
uint32_t debug_server_recv(void *buf, uint32_t len);

enum DEBUG_SERVER_MESSAGE_TYPES {
  MESSAGE_MESSAGE,
  MESSAGE_VMEXIT,
  MESSAGE_EXEC_CONTINUE,
  MESSAGE_EXEC_STEP,
  MESSAGE_MEMORY_READ,
  MESSAGE_MEMORY_DATA,
  MESSAGE_MEMORY_WRITE,
  MESSAGE_MEMORY_WRITE_COMMIT,
  MESSAGE_CORE_REGS_READ,
  MESSAGE_CORE_REGS_DATA
};

//
// Registers structures
//

typedef struct _core_regs {
  // GPRs
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  // Segment
  uint16_t cs;
  uint16_t ds;
  uint16_t ss;
  uint16_t es;
  uint16_t fs;
  uint16_t gs;
  // Pointer
  uint64_t rbp;
  uint64_t rsp;
  // Index
  uint64_t rsi;
  uint64_t rdi;
  // Instruction pointer
  uint64_t rip;
  // Control registers
  uint64_t cr0;
  uint64_t cr1;
  uint64_t cr2;
  uint64_t cr3;
  uint64_t cr4;
} __attribute__((packed)) core_regs;

//
// Messages
//

typedef struct _message {
  uint8_t type;
  uint8_t core;
} __attribute__((packed)) message;

typedef struct _message_vmexit {
  uint8_t type;
  uint8_t core;
  uint32_t exit_reason;
} __attribute__((packed)) message_vmexit;

typedef struct _message_memory_read {
  uint8_t type;
  uint8_t core;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_read;

typedef struct _message_memory_data {
  uint8_t type;
  uint8_t core;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_data;

typedef struct _message_memory_write {
  uint8_t type;
  uint8_t core;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_write;

typedef struct _message_memory_write_commit {
  uint8_t type;
  uint8_t core;
  uint8_t ok;
} __attribute__((packed)) message_memory_write_commit;

typedef struct _message_core_regs_read {
  uint8_t type;
  uint8_t core;
} __attribute__((packed)) message_core_regs_read;

typedef struct _message_core_regs_data {
  uint8_t type;
  uint8_t core;
  // Registers
  core_regs regs;
} __attribute__((packed)) message_core_regs_data;

static inline void *message_check_type(message *m, uint8_t type) {
  if (m->type == type) {
    return m;
  } else {
    return NULL;
  }
}

void debug_server_run(uint32_t exit_reason, struct registers *regs);

static inline uint8_t debug_server_get_core() {
  // XXX as dirty as possible
  return 0;
}

extern protocol_82579LM *eth;

#endif
