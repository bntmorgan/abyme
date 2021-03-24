/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DEBUG_SERVER_H__
#define __DEBUG_SERVER_H__
#include "efi/efi_82579LM.h"
#include "vmm.h"
#include "debug_protocol.h"

void debug_server_init();
void debug_server_send(void *buf, uint32_t len);
int32_t debug_server_recv(void *buf, uint32_t len);
void debug_server_putc(uint8_t value);
void debug_server_enable_putc();
void debug_server_disable_putc();
void debug_server_vmexit(uint8_t vmid, uint32_t exit_reason,
    struct registers *guest_regs);
void debug_server_panic(uint8_t vmid, uint64_t code, uint64_t extra,
    struct registers *guest_regs);
void set_mtf(void);
void debug_server_reboot(void);

extern uint8_t debug_server;
extern uint8_t debug_printk;
extern uint32_t debug_server_level;

static inline void *message_check_type(message *m, uint8_t type) {
  if (m->type == type) {
    return m;
  } else {
    return NULL;
  }
}

typedef struct _message_send_debug {
  uint8_t type;
  uint8_t vmid;
  uint8_t send_debug[VM_NB][NB_EXIT_REASONS];
} __attribute__((packed)) message_send_debug;

void debug_server_run(struct registers *regs);

extern protocol_82579LM *eth;

// 8 * 0x20000 = 1Mo must be DEBUG_SERVER_CR3_PER_MESSAGE multiple
// #define DEBUG_SERVER_CR3_SIZE 0x200000
#define DEBUG_SERVER_CR3_SIZE 512
#define DEBUG_SERVER_CR3_PER_MESSAGE 128
void debug_server_log_cr3_add(struct registers *regs, uint64_t cr3);

void debug_server_mtf(void);

void debug_server_send_debug_all(void);
void debug_server_send_debug_unset(uint8_t reason);
void debug_server_send_debug_set(uint8_t reason);

#endif
