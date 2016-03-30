#ifndef __DEBUG_PROTOCOL_H__
#define __DEBUG_PROTOCOL_H__

enum DEBUG_SERVER_MESSAGE_TYPES {
  MESSAGE_MESSAGE,
  MESSAGE_VMEXIT,
  MESSAGE_EXEC_CONTINUE,
  MESSAGE_INFO,
  MESSAGE_MEMORY_READ,
  MESSAGE_MEMORY_DATA,
  MESSAGE_MEMORY_WRITE,
  MESSAGE_COMMIT,
  MESSAGE_CORE_REGS_READ,
  MESSAGE_CORE_REGS_DATA,
  MESSAGE_UNHANDLED_VMEXIT,
  MESSAGE_VMCS_READ,
  MESSAGE_VMCS_DATA,
  MESSAGE_VMCS_WRITE,
  MESSAGE_USER_DEFINED,
  MESSAGE_SEND_DEBUG,
  MESSAGE_NETBOOT,
  MESSAGE_RESET
};

enum DEBUG_SERVER_USER_DEFINED_TYPES {
  USER_DEFINED_LOG_CR3,
  USER_DEFINED_LOG_MD5
};

//
// Registers structures
//

struct core_regs {
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
  // Flags
  uint64_t rflags;
  // MSRs 
  uint64_t ia32_efer;
} __attribute__((packed));

//
// Messages
//

typedef struct _message {
  uint8_t type;
  uint8_t vmid;
} __attribute__((packed)) message;

typedef struct _message_vmexit {
  uint8_t type;
  uint8_t vmid;
  uint32_t exit_reason;
  // Registers
  struct core_regs regs;
} __attribute__((packed)) message_vmexit;

typedef struct _message_memory_read {
  uint8_t type;
  uint8_t vmid;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_read;

typedef struct _message_memory_data {
  uint8_t type;
  uint8_t vmid;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_data;

typedef struct _message_memory_write {
  uint8_t type;
  uint8_t vmid;
  uint64_t address;
  uint64_t length;
} __attribute__((packed)) message_memory_write;

typedef struct _message_commit {
  uint8_t type;
  uint8_t vmid;
  uint8_t ok;
} __attribute__((packed)) message_commit;

typedef struct _message_core_regs_read {
  uint8_t type;
  uint8_t vmid;
} __attribute__((packed)) message_core_regs_read;

typedef struct _message_core_regs_data {
  uint8_t type;
  uint8_t vmid;
  // Registers
  struct core_regs regs;
} __attribute__((packed)) message_core_regs_data;

typedef struct _message_vmcs_read {
  uint8_t type;
  uint8_t vmid;
  uint8_t shadow;
} __attribute__((packed)) message_vmcs_read;

typedef struct _message_vmcs_data {
  uint8_t type;
  uint8_t vmid;
  uint8_t shadow;
} __attribute__((packed)) message_vmcs_data;

typedef struct _message_vmcs_write {
  uint8_t type;
  uint8_t vmid;
} __attribute__((packed)) message_vmcs_write;

typedef struct _message_user_defined {
  uint8_t type;
  uint8_t vmid;
  uint16_t user_type;
  uint64_t length;
} __attribute__((packed)) message_user_defined;

typedef struct _netboot {
  uint8_t type;
  uint8_t vmid;
} __attribute__((packed)) message_netboot;

typedef struct _message_reset {
  uint8_t type;
  uint8_t vmid;
} __attribute__((packed)) message_reset;

typedef struct _message_info {
  uint8_t type;
  uint8_t vmid;
  uint64_t length;
} __attribute__((packed)) message_info;

#endif//__DEBUG_PROTOCOL_H__
