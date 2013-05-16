#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "string.h"
#include "vmm.h"
#include "vmcs.h"
#include "cpu.h"
#include "debug_server.h"
  
protocol_82579LM *eth;

void debug_server_init() {
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID; 

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }
  printk("BAR0 %X\n", eth->bar0);
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
  memcpy((uint8_t *)mr->address, b, length);
  // We look if the first byte has been successfully written
  uint8_t ok = *((uint8_t *)mr->address) == b[0];
  message_memory_write_commit r = {
    MESSAGE_MEMORY_WRITE_COMMIT,
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

void debug_server_create_regs(message_vmexit *m, struct registers *regs) {
  // Copy segment regs
  debug_server_get_segment_regs(&m->regs); 
  // Copy GPRs rsp, rbp, rsi, rdi and rip
  m->regs.rax = regs->rax;
  m->regs.rbx = regs->rbx;
  m->regs.rcx = regs->rcx;
  m->regs.rdx = regs->rdx;
  m->regs.r8 = regs->r8;
  m->regs.r9 = regs->r9;
  m->regs.r10 = regs->r10;
  m->regs.r11 = regs->r11;
  m->regs.r12 = regs->r12;
  m->regs.r13 = regs->r13;
  m->regs.r14 = regs->r14;
  m->regs.r15 = regs->r15;
  // Copy index registers
  m->regs.rsi = regs->rsi;
  m->regs.rdi = regs->rdi;
  // Copy pointer registers
  m->regs.rbp = regs->rbp;
  m->regs.rsp = regs->rsp;
  // Copy instruction pointer register
  m->regs.rip = regs->rip;
}

void debug_server_run(uint32_t exit_reason, struct registers *regs) {
  message_vmexit ms = {
    MESSAGE_VMEXIT,
    debug_server_get_core(),
    exit_reason
  };
  debug_server_create_regs(&ms, regs);
  debug_server_send(&ms, sizeof(ms));
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
