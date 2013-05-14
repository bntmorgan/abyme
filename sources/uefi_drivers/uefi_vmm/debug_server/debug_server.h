#ifndef __DEBUG_SERVER_H__
#define __DEBUG_SERVER_H__
#include "efi/efi_82579LM.h"

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
  MESSAGE_MEMORY_WRITE_COMMIT
};

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

static inline void *message_check_type(message *m, uint8_t type) {
  if (m->type == type) {
    return m;
  } else {
    return NULL;
  }
}

void debug_server_run(uint32_t exit_reason);

static inline uint8_t debug_server_get_core() {
  // XXX as dirty as possible
  return 0;
}

extern protocol_82579LM *eth;

#endif
