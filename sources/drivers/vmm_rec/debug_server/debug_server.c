#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "string.h"
#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "cpu.h"
#include "debug_server.h"

#define MAX_INFO_SIZE 1024

extern void (*putc)(uint8_t);

protocol_82579LM *eth;

uint8_t debug_server = 0;
uint8_t debug_printk = 0;

uint32_t debug_server_level = 0;

static uint8_t send_debug[NB_EXIT_REASONS];

/**
 * Logging
 */
uint64_t log_cr3_table[DEBUG_SERVER_CR3_SIZE];
uint64_t log_cr3_index;

void debug_server_log_cr3_flush(struct registers *regs) {
  // INFO("Flush\n");
  int i;
  uint8_t b[sizeof(message_user_defined) + DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t)];
  message_user_defined *m = (message_user_defined *)&b[0];
  uint8_t *data = b + sizeof(message_user_defined);
  m->type = MESSAGE_USER_DEFINED;
  m->core = debug_server_get_core();
  m->user_type = USER_DEFINED_LOG_CR3;
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

void debug_server_vmexit(uint8_t core, uint32_t exit_reason,
    struct registers *guest_regs) {
  if (send_debug[exit_reason]) {
    message_vmexit ms = {
      MESSAGE_VMEXIT,
      core,
      exit_reason
    };
    debug_server_send(&ms, sizeof(ms));
    debug_server_run(guest_regs);
  }
}

void debug_server_panic(uint8_t core, uint64_t code, uint64_t extra,
    struct registers *guest_regs) {
  message_vmm_panic m = {
    MESSAGE_VMM_PANIC,
    core,
    code,
    extra
  };
  debug_server_send(&m, sizeof(m));
  if (guest_regs != NULL) {
    debug_server_run(guest_regs);
  } else {
    while(1);
  }
}

void debug_server_init() {
  /* Init exit reasons for which we need to send a debug message */
  memset(&send_debug[0], 1, NB_EXIT_REASONS);
  send_debug[EXIT_REASON_CPUID] = 0;
  send_debug[EXIT_REASON_IO_INSTRUCTION] = 0;
  send_debug[EXIT_REASON_WRMSR] = 0;
  send_debug[EXIT_REASON_RDMSR] = 0;
  send_debug[EXIT_REASON_XSETBV] = 0;
  send_debug[EXIT_REASON_CR_ACCESS] = 0;
  send_debug[EXIT_REASON_INVVPID] = 0;
  send_debug[EXIT_REASON_VMRESUME] = 0;
  send_debug[EXIT_REASON_VMREAD] = 0;
  send_debug[EXIT_REASON_VMWRITE] = 0;

  debug_server_log_cr3_reset();
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
  } else {
    debug_printk = 1;
    debug_server_level = eth->get_level();
    INFO("VMM level %d\n", debug_server_level);
    if (debug_server_level == 0) {
      INFO("Debug server enabled\n");
      debug_server = 1;
    }
//     INFO("Uninstalling efi interface\n");
//     eth->uninstall();
    printk("DEBUG SERVER INIT : ETH BAR0 %X\n", eth->bar0);
  }
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

void debug_server_handle_vmcs_write(message_vmcs_write *mr) {
  uint8_t *data = (uint8_t*)mr + sizeof(message_vmcs_write);
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

void debug_server_handle_send_debug(message_send_debug *mr) {
  uint32_t i;
  for (i = 0; i < NB_EXIT_REASONS; i++) {
    send_debug[i] = mr->send_debug[i];
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
          debug_server_handle_vmcs_write((message_vmcs_write*)mr);
          break;
        case MESSAGE_SEND_DEBUG:
          debug_server_handle_send_debug((message_send_debug*)mr);
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

void debug_server_putc(uint8_t value) {
  static uint8_t b [MAX_INFO_SIZE + sizeof(message_info)];
  static message_info *m = (message_info *)b;
  static uint8_t *buf = (uint8_t *)&b[0] + sizeof(message_info);
  static uint8_t current_size = 0;

  buf[current_size] = value;
  current_size++;

  if (value == '\n' || current_size == MAX_INFO_SIZE) {
    m->type = MESSAGE_INFO;
    m->core = debug_server_get_core();
    m->length = current_size;
    m->level = debug_server_level;
    eth->send(b, current_size + sizeof(message_info), EFI_82579LM_API_BLOCK);
    current_size = 0;
  }
}

// change stdio putc pointer to our handler
void debug_server_enable_putc() {
  putc = &debug_server_putc;
}

// change stdio putc pointer to our handler
void no_putc(uint8_t value) {}
void debug_server_disable_putc() {
  putc = &no_putc;
}
