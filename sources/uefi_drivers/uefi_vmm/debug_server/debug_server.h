#ifndef __DEBUG_SERVER_H__
#define __DEBUG_SERVER_H__

void debug_server_init();
void debug_server_send(void *buf, uint32_t len);
uint32_t debug_server_recv(void *buf, uint32_t len);

enum DEBUG_SERVER_MESSAGE_TYPES {
  MESSAGE_MESSAGE,
  MESSAGE_VMEXIT,
  MESSAGE_EXEC_CONTINUE,
  MESSAGE_EXEC_STEP
};

typedef struct _message {
  uint8_t type;
} message;

typedef struct _message_vmexit {
  uint8_t type;
  uint32_t exit_reason;
} message_vmexit;

static inline void *message_check_type(message *m, uint8_t type) {
  if (m->type == type) {
    return m;
  } else {
    return NULL;
  }
}

void debug_server_run(uint32_t exit_reason);

#endif
