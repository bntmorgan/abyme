#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "string.h"
#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "cpu.h"
#include "debug_server.h"
  
protocol_82579LM *eth;

/**
 * Logging
 */
uint64_t log_cr3_table[DEBUG_SERVER_CR3_SIZE];
uint64_t log_cr3_index;

void debug_server_log_cr3_flush(struct registers *regs) {
  // INFO("Flush\n");
  int i;
  uint8_t b[sizeof(message_log_cr3) + DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t)];
  message_log_cr3 *m = (message_log_cr3 *)&b[0];
  uint8_t *data = b + sizeof(message_log_cr3);
  m->type = MESSAGE_LOG_CR3;
  m->core = debug_server_get_core();
  m->length = DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t);
  for (i = 0; i < DEBUG_SERVER_CR3_SIZE / DEBUG_SERVER_CR3_PER_MESSAGE; i++) {
    // Copy DEBUG_SERVER_CR3_PER_MESSAGE cr3
    memcpy(data, &log_cr3_table[i * DEBUG_SERVER_CR3_PER_MESSAGE], DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t));
    // Send the message
    // INFO("Send\n");
    debug_server_send(b, sizeof(b));
    // Run the debug server
    // INFO("Run\n");
    // debug_server_run(regs);
  }
}

void debug_server_log_cr3_reset() {
  log_cr3_index = 0;
}

void debug_server_log_cr3_add(struct registers *regs, uint64_t cr3) {
  // INFO("Add\n");
  static uint64_t prec = 0;
  if (cr3 == prec) {
    return;
  }
  prec = cr3;
  if (log_cr3_index == DEBUG_SERVER_CR3_SIZE) {
    debug_server_log_cr3_flush(regs);
    debug_server_log_cr3_reset();
  }
  log_cr3_table[log_cr3_index] = cr3;
  log_cr3_index++;
}

void debug_server_init() {
  debug_server_log_cr3_reset();
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID; 

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }
  printk("DEBUG SERVER INIT : ETH BAR0 %X\n", eth->bar0);
}

void debug_server_eth_init() {
  eth->init();
}

void debug_server_handle_memory_read(message_memory_read *mr) {
  uint64_t length = (mr->length + sizeof(message_memory_data) > eth->mtu) ? eth->mtu - sizeof(message_memory_data) : mr->length;
  // Handle message memory request
  uint8_t b[length + sizeof(message_memory_data)];
  message_memory_data *r = (message_memory_data *)b;
  r->type = MESSAGE_MEMORY_DATA;
  r->core = debug_server_get_core();
  r->address = mr->address;
  r->length = length;
  uint8_t *buf = (uint8_t *)&b[0] + sizeof(message_memory_data);
  memcpy(buf, (uint8_t *)((uintptr_t)mr->address), length);
  debug_server_send(b, sizeof(b));
}

void debug_server_handle_memory_write(message_memory_write *mr) {
  // We don't trust the length in the received message
  uint64_t length = (mr->length + sizeof(message_memory_write) > eth->mtu) ? eth->mtu - sizeof(message_memory_write) : mr->length;
  uint8_t *b = (uint8_t *)mr + sizeof(message_memory_write);

  uint8_t ok;
  memcpy((uint8_t *)mr->address, b, length);
  ok = *((uint8_t *)mr->address) == b[0];

  message_commit r = {
    MESSAGE_COMMIT,
    debug_server_get_core(),
    ok
  };
  debug_server_send(&r, sizeof(r));
}

void debug_server_get_segment_regs(core_regs *regs) {
  regs->cs = cpu_vmread(GUEST_CS_SELECTOR);
  regs->ds = cpu_vmread(GUEST_DS_SELECTOR);
  regs->ss = cpu_vmread(GUEST_SS_SELECTOR);
  regs->es = cpu_vmread(GUEST_ES_SELECTOR);
  regs->fs = cpu_vmread(GUEST_FS_SELECTOR);
  regs->gs = cpu_vmread(GUEST_GS_SELECTOR);
}

void debug_server_get_control_regs(core_regs *regs) {
  regs->cr0 = cpu_vmread(GUEST_CR0);
  regs->cr1 = 0;
  regs->cr2 = 0;
  regs->cr3 = cpu_vmread(GUEST_CR3);
  regs->cr4 = cpu_vmread(GUEST_CR4);
}

void debug_server_handle_core_regs_read(message_core_regs_read *mr, struct registers *regs) {
  message_core_regs_data m = {
    MESSAGE_CORE_REGS_DATA,
    debug_server_get_core()
  };
  // Copy segment regs
  debug_server_get_segment_regs(&m.regs); 
  // Copy control registers
  debug_server_get_control_regs(&m.regs); 
  // Copy GPRs rsp, rbp, rsi, rdi and rip
  m.regs.rax = regs->rax;
  m.regs.rbx = regs->rbx;
  m.regs.rcx = regs->rcx;
  m.regs.rdx = regs->rdx;
  m.regs.r8 = regs->r8;
  m.regs.r9 = regs->r9;
  m.regs.r10 = regs->r10;
  m.regs.r11 = regs->r11;
  m.regs.r12 = regs->r12;
  m.regs.r13 = regs->r13;
  m.regs.r14 = regs->r14;
  m.regs.r15 = regs->r15;
  // Copy index registers
  m.regs.rsi = regs->rsi;
  m.regs.rdi = regs->rdi;
  // Copy pointer registers
  m.regs.rbp = regs->rbp;
  m.regs.rsp = regs->rsp;
  // Copy instruction pointer register
  m.regs.rip = regs->rip;
  // Send to the client
  debug_server_send(&m, sizeof(m));
}

void debug_server_handle_vmcs_read(message_vmcs_read *mr) {
  uint8_t size = sizeof(message_vmcs_data);
  uint8_t s = -1;
  uint64_t e = 0;
  uint8_t *data = (uint8_t *)mr + sizeof(message_vmcs_read);
  uint8_t b[eth->mtu];
  uint8_t *buf = &b[0];
  uint64_t v = 0;
  message_vmcs_data *m = (message_vmcs_data *)&buf[0];
  m->core = debug_server_get_core();
  m->type = MESSAGE_VMCS_DATA;
  buf = (uint8_t *)buf + sizeof(message_vmcs_data); 
  // Size
  s = data[0];
  data++;
  // Global size
  size += sizeof(s) + sizeof(uint64_t) + s;
  while (s && size < eth->mtu) {
    // Encoding
    e = *((uint64_t *)data);
    data = (uint8_t *)((uint64_t *)data + 1);
    // Size
    buf[0] = s;
    buf++;
    // Encoding
    *((uint64_t *)buf) = e;
    buf = (uint8_t *)buf + sizeof(uint64_t);
    // Field
    v = cpu_vmread(e);
    memcpy(buf, (void *)&v, s);
    buf = (uint8_t *)buf + s;
    // Size
    s = data[0];
    data++;
    // Global size
    size += sizeof(s) + sizeof(uint64_t) + s;
  }
  // Ends the message
  buf[0] = 0;
  // Send the message
  debug_server_send(b, size);
}

void debug_server_handle_vmcs_write(message_vmcs_read *mr) {
  uint8_t *data = (uint8_t*)mr + sizeof(message_vmcs_read);
  // Size
  uint8_t s = *((uint8_t*)data);
  data += 1;
  uint64_t e = 0;
  uint64_t v = 0;
  while (s) {
    // Encoding
    e = *((uint64_t *)data);
    data += 8;
    // Value
    if (s == 2) {
      v = *((uint16_t *)data);
      data += 2;
    } else if (s == 4) {
      v = *((uint32_t *)data);
      data += 4;
    } else if (s == 8) {
      v = *((uint64_t *)data);
      data += 8;
    } else {
      return;
    }
    // Write the vmcs fields
    cpu_vmwrite(e, v);
    // Size
    s = *((uint8_t *)data);
    data += 1;
  }
  message_commit r = {
    MESSAGE_COMMIT,
    debug_server_get_core(),
    1
  };
  debug_server_send(&r, sizeof(r));
}

void debug_server_run(struct registers *regs) {
  uint8_t buf[eth->mtu];
  message *mr = (message *)buf;
  mr->type = MESSAGE_MESSAGE;
  while (mr->type != MESSAGE_EXEC_CONTINUE) {
    if (debug_server_recv(mr, eth->mtu) == -1) {
      mr->type = MESSAGE_MESSAGE;
      continue;
    } else {
      // We have received a message
      switch (mr->type) {
        case MESSAGE_MEMORY_READ:
          debug_server_handle_memory_read((message_memory_read*)mr);
          break;
        case MESSAGE_MEMORY_WRITE:
          debug_server_handle_memory_write((message_memory_write*)mr);
          break;
        case MESSAGE_CORE_REGS_READ:
          debug_server_handle_core_regs_read((message_core_regs_read*)mr, regs);
          break;
        case MESSAGE_VMCS_READ:
          debug_server_handle_vmcs_read((message_vmcs_read*)mr);
          break;
        case MESSAGE_VMCS_WRITE:
          debug_server_handle_vmcs_write((message_vmcs_read*)mr);
          break;
        default: {
          // nothing
        }
      }
    }
  }
}

void debug_server_send(void *buf, uint32_t len) {
  eth->send(buf, len, EFI_82579LM_API_BLOCK);
}

uint32_t debug_server_recv(void *buf, uint32_t len) {
  return eth->recv(buf, len, EFI_82579LM_API_BLOCK);
}
