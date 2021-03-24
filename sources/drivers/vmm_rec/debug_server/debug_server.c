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

#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "debug.h"
#include "string.h"
#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "cpu.h"
#include "debug_server.h"
#include "efiw.h"
#include "debug_protocol.h"
#include "gdt.h"
#include "microudp.h"

#define MAX_INFO_SIZE 1024

// XXX
uint8_t buf2[ETHERNET_SIZE];
union ethernet_buffer *buffer;
void clear_buffer() {
  memset(&buf2[0], 0, ETHERNET_SIZE);
  buffer = (union ethernet_buffer *)&buf2[0];
}

extern void (*putc)(uint8_t);

protocol_82579LM *eth;

uint8_t debug_server = 0;
uint8_t debug_printk = 0;

uint32_t debug_server_level = 0;

static uint8_t send_debug[VM_NB][NB_EXIT_REASONS];

// Messages buffer
static uint8_t *rb = NULL;
static uint8_t *sb = NULL;

/**
 * Logging
 */
uint64_t log_cr3_table[DEBUG_SERVER_CR3_SIZE];
uint64_t log_cr3_index;

void debug_server_log_cr3_flush(struct registers *regs) {
  // INFO("Flush\n");
  int i;
  message_user_defined *m = (message_user_defined *)&sb[0];
  uint8_t *data = sb + sizeof(message_user_defined);
  m->type = MESSAGE_USER_DEFINED;
  m->vmid = vm->index;
  m->user_type = USER_DEFINED_LOG_CR3;
  m->length = DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t);
  for (i = 0; i < DEBUG_SERVER_CR3_SIZE / DEBUG_SERVER_CR3_PER_MESSAGE; i++) {
    // Copy DEBUG_SERVER_CR3_PER_MESSAGE cr3
    memcpy(data, &log_cr3_table[i * DEBUG_SERVER_CR3_PER_MESSAGE],
        DEBUG_SERVER_CR3_PER_MESSAGE * sizeof(uint64_t));
    // Send the message
    // INFO("Send\n");
    debug_server_send(sb, sizeof(message_user_defined) + m->length);
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

void debug_server_get_segment_regs(struct core_regs *regs) {
  VMR2(gs.cs_selector, regs->cs);
  VMR2(gs.ds_selector, regs->ds);
  VMR2(gs.ss_selector, regs->ss);
  VMR2(gs.es_selector, regs->es);
  VMR2(gs.fs_selector, regs->fs);
  VMR2(gs.gs_selector, regs->gs);
}

void debug_server_get_control_regs(struct core_regs *regs) {
  VMR2(gs.cr0, regs->cr0);
  regs->cr1 = 0;
  regs->cr2 = cpu_read_cr2();
  VMR2(gs.cr3, regs->cr3);
  VMR2(gs.cr4, regs->cr4);
}

void debug_server_core_regs_read(struct core_regs *cr, struct registers *regs) {
  // Copy segment regs
  debug_server_get_segment_regs(cr);
  // Copy control registers
  debug_server_get_control_regs(cr);
  // Copy GPRs rsp, rbp, rsi, rdi and rip
  cr->rax = regs->rax;
  cr->rbx = regs->rbx;
  cr->rcx = regs->rcx;
  cr->rdx = regs->rdx;
  cr->r8 = regs->r8;
  cr->r9 = regs->r9;
  cr->r10 = regs->r10;
  cr->r11 = regs->r11;
  cr->r12 = regs->r12;
  cr->r13 = regs->r13;
  cr->r14 = regs->r14;
  cr->r15 = regs->r15;
  // Copy index registers
  cr->rsi = regs->rsi;
  cr->rdi = regs->rdi;
  // Copy pointer registers
  cr->rbp = regs->rbp;
  cr->rsp = regs->rsp;
  // Copy instruction pointer register
  cr->rip = regs->rip;
  // Flasgs
  VMR2(gs.rflags, cr->rflags);
  // MSRs
  VMR2(gs.ia32_efer, cr->ia32_efer);
}

void debug_server_vmexit(uint8_t vmid, uint32_t exit_reason,
    struct registers *guest_regs) {
  if (send_debug[vmid][exit_reason] || exit_reason == EXIT_REASON_VMLAUNCH) {
    message_vmexit *ms = (message_vmexit*)&sb[0];
    ms->type = MESSAGE_VMEXIT;
    ms->vmid = vmid;
    ms->exit_reason = exit_reason;
    debug_server_core_regs_read(&ms->regs, guest_regs);
    debug_server_send(ms, sizeof(message_vmexit));
    debug_server_run(guest_regs);
  }
}

void debug_server_init() {
//  uint32_t i;

  // Info message buffer
  rb = efi_allocate_pages(1);
  sb = efi_allocate_pages(1);

  /* Init exit reasons for which we need to send a debug message */
  memset(&send_debug[0][0], 1, NB_EXIT_REASONS * VM_NB);
//  for (i = 0; i < VM_NB; i++) {
//    send_debug[i][EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED] = 1;
//    send_debug[i][EXIT_REASON_EPT_VIOLATION] = 1;
//    send_debug[i][EXIT_REASON_HLT] = 1;
//    send_debug[i][EXIT_REASON_TRIPLE_FAULT] = 1;
//    send_debug[i][EXIT_REASON_MWAIT] = 1;
//    send_debug[i][EXIT_REASON_MONITOR_TRAP_FLAG] = 1;
//    send_debug[i][EXIT_REASON_VMCALL] = 1;
//    send_debug[i][EXIT_REASON_IO_INSTRUCTION] = 1;
//    // XXX
//    send_debug[i][EXIT_REASON_CR_ACCESS] = 1;
//  }


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
    printk("DEBUG SERVER INIT : ETH BAR0 %X\n", eth->bar0);

    // XXX
    uint16_t len;
    clear_buffer();
    microudp_start(eth->mac_addr, SERVER_IP);

    INFO("ARP request send\n");
    len=microudp_start_arp(buffer, CLIENT_IP, ARP_OPCODE_REQUEST);
    eth->eth_send(buf2, len, 1);

    clear_buffer();

    eth->eth_recv(buf2, 1500, 1);
    microudp_handle_frame(buffer);
    clear_buffer();
  }
}

void debug_server_handle_memory_read(message_memory_read *mr) {
  uint64_t length = (mr->length + sizeof(message_memory_data) > eth->mtu) ?
    eth->mtu - sizeof(message_memory_data) : mr->length;
  // Handle message memory request
  message_memory_data *r = (message_memory_data *)sb;
  r->type = MESSAGE_MEMORY_DATA;
  r->vmid = vm->index;
  r->address = mr->address;
  r->length = length;
  uint8_t *buf = (uint8_t *)&sb[0] + sizeof(message_memory_data);
  memcpy(buf, (uint8_t *)((uintptr_t)mr->address), length);
  debug_server_send(sb, length + sizeof(message_memory_data));
}

void debug_server_handle_memory_write(message_memory_write *mr) {
  // We don't trust the length in the received message
  uint64_t length = (mr->length + sizeof(message_memory_write) > eth->mtu) ?
    eth->mtu - sizeof(message_memory_write) : mr->length;

  uint8_t *b = (uint8_t *)mr + sizeof(message_memory_write);
  memcpy((uint8_t *)mr->address, b, length);

  // XXX flush wb caches
  __asm__ __volatile__("wbinvd");

  message_commit *r = (message_commit*)&sb[0];
  r->type = MESSAGE_COMMIT;
  r->vmid = vm->index;
  r->ok = 1;

  debug_server_send(sb, sizeof(message_commit));
}

void debug_server_handle_core_regs_read(message_core_regs_read *mr, struct
    registers *regs) {
  message_core_regs_data *m = (message_core_regs_data*) &sb[0];
  m->type = MESSAGE_CORE_REGS_DATA,
  m->vmid = vm->index;
  debug_server_core_regs_read(&m->regs, regs);
  // Send to the client
  debug_server_send(m, sizeof(message_core_regs_data));
}

void debug_server_handle_vmcs_read(message_vmcs_read *mr) {
  uint8_t size = sizeof(message_vmcs_data);
  uint8_t s = -1;
  uint64_t e = 0;
  uint8_t *data = (uint8_t *)mr + sizeof(message_vmcs_read);
  uint8_t *buf = &sb[0];
  uint64_t v = 0;
  uint8_t *current_vmcs = cpu_vmptrst();
  uint8_t *read_vmcs;
  message_vmcs_data *m = (message_vmcs_data *)&buf[0];
  m->type = MESSAGE_VMCS_DATA;
  if (mr->vmid < VM_NB) {
    // TODO XXX
    if (mr->shadow) {
      if (vm_pool[mr->vmid].shadow_vmcs == 0) {
        read_vmcs = &vm_pool[mr->vmid].vmcs_region[0];
        mr->shadow = 0;
      } else {
        read_vmcs = vm_pool[mr->vmid].shadow_vmcs;
      }
    } else {
      read_vmcs = &vm_pool[mr->vmid].vmcs_region[0];
    }
    m->vmid = mr->vmid;
  } else {
    read_vmcs = vm->vmcs_region;
    m->vmid = vm->index;
  }
  m->shadow = mr->shadow;
  buf = (uint8_t *)buf + sizeof(message_vmcs_data);
  // Size
  s = data[0];
  data++;
  // Global size
  size += sizeof(s) + sizeof(uint64_t) + s;
  // load the requested vmcs
  cpu_vmptrld(read_vmcs);
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
  // reload the current vmcs
  cpu_vmptrld(current_vmcs);
  // Ends the message
  buf[0] = 0;
  // Send the message
  debug_server_send(sb, size);
}

void debug_server_handle_vmcs_write(message_vmcs_write *mr,
    struct registers *guest_regs) {
  uint8_t *data = (uint8_t*)mr + sizeof(message_vmcs_write);
  // Size
  uint8_t s = *((uint8_t*)data);
  data += 1;
  uint64_t e = 0;
  uint64_t v = 0;
  uint8_t *current_vmcs = cpu_vmptrst();
  uint8_t *write_vmcs;
  uint8_t vmid = 0;

  if (mr->vmid < VM_NB) {
    write_vmcs = &vm_pool[mr->vmid].vmcs_region[0];
    vmid = mr->vmid;
  } else {
    write_vmcs = vm->vmcs_region;
    vmid = vm->index;
  }
  cpu_vmptrld(write_vmcs);
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
    // Special case for monitor trap flag handling
    if (e == CPU_BASED_VM_EXEC_CONTROL) {
      if (v & MONITOR_TRAP_FLAG) {
        // INFO("Monitor trap flag activation\n");
        vmm_mtf_set();
      } else {
        // INFO("Monitor trap flag deactivation\n");
        vmm_mtf_unset();
      }
    }
    // Special case for some guest regs
    if (e == GUEST_RIP) {
      guest_regs->rip = v;
    } else if (e == GUEST_RSP) {
      guest_regs->rsp = v;
    }
    // Size
    s = *((uint8_t *)data);
    data += 1;
  }
  cpu_vmptrld(current_vmcs);

  message_commit *r = (message_commit*)&sb[0];
  r->type = MESSAGE_COMMIT;
  r->vmid = vmid;
  r->ok = 1;
  debug_server_send(sb, sizeof(message_commit));
}

void debug_server_handle_reset(message_reset *mr) {
  INFO("RESET the system\n");
  REBOOT;
}

void debug_server_send_debug_unset(uint8_t reason) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    send_debug[i][reason] = 0;
  }
}

void debug_server_send_debug_set(uint8_t reason) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    send_debug[i][reason] = 1;
  }
}

void debug_server_send_debug_all(void) {
  memset(&send_debug[0][0], 1, NB_EXIT_REASONS * VM_NB);
}

void debug_server_handle_send_debug(message_send_debug *mr) {
  memcpy(&send_debug[0][0], &mr->send_debug[0][0], NB_EXIT_REASONS * VM_NB);

  message_commit *r = (message_commit*)&sb[0];
  r->type = MESSAGE_COMMIT;
  r->vmid = vm->index;
  r->ok = 1;

  debug_server_send(sb, sizeof(message_commit));
}

void debug_server_run(struct registers *regs) {
  int32_t ret;
  char lol = 'a';;
  message *mr = (message *)rb;
  mr->type = MESSAGE_MESSAGE;
  while (mr->type != MESSAGE_EXEC_CONTINUE) {
    ret = debug_server_recv(mr, eth->mtu);
    if (lol == 'z') {
      lol = 'a';
    } else {
      lol++;
    }
    if (ret == -1) {
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
          debug_server_handle_vmcs_write((message_vmcs_write*)mr, regs);
          break;
        case MESSAGE_SEND_DEBUG:
          debug_server_handle_send_debug((message_send_debug*)mr);
          break;
        case MESSAGE_RESET:
          debug_server_handle_reset((message_reset*)mr);
          break;
        default: {
          // nothing
        }
      }
    }
  }
}

void debug_server_send(void *buf, uint32_t len) {
  uint16_t size=microudp_start_arp(buffer, CLIENT_IP, ARP_OPCODE_REQUEST);
  eth->eth_send(buf2, size, 1);

  clear_buffer();

  len = microudp_fill(buffer, 6666, 6666, buf, len);
  eth->eth_send(buf2, len, 1);
  clear_buffer();
}

int32_t debug_server_recv(void *buf, uint32_t len) {
  clear_buffer();
  uint64_t tsc = cpu_rdtsc();
  uint32_t size = eth->eth_recv(buf2, len, 1);
  uint32_t payload_size = size - sizeof(struct udp_frame) - sizeof(struct ethernet_header);
  // Handle ip stuff
  uint32_t size2 = microudp_handle_frame(buffer);
  // Size is > 0 we have to send a frame back to the sender machine
  if (size2 > 0) {
    eth->eth_send(buffer, size2, 1);
  }
  SERIAL_INFO("0x%016X : Size of frame : 0x%x \n", tsc, size);
  SERIAL_DUMP((void *)buf2, 4, size, 8, 0, 6, 0);
  SERIAL_NEW_LINE;
  if (buffer->frame.eth_header.ethertype == ntohs(ETHERTYPE_IP) &&
      // buffer->frame.contents.udp.ip.proto == htons(0x11) &&
      buffer->frame.contents.udp.udp.dst_port == htons(6666)) {
    memcpy(buf, &buffer->frame.contents.udp.payload[0], payload_size);
    SERIAL_INFO("This is 6666 udp protocol\n");
    return payload_size;
  } else {
    SERIAL_INFO("This is not 6666 protocol!\n");
    return -1;
  }
}

void debug_server_putc(uint8_t value) {
  message_info *m = (message_info *)sb;
  uint8_t *buf = (uint8_t *)&sb[0] + sizeof(message_info);
  static uint8_t current_size = 0;

  buf[current_size] = value;
  current_size++;

  if (value == '\n' || current_size == MAX_INFO_SIZE) {
    m->type = MESSAGE_INFO;
    m->vmid = debug_server_level;
    m->length = current_size;

    // XXX
    clear_buffer();
    uint16_t len = microudp_fill(buffer, 6666, 6666, sb, current_size +
        sizeof(message_info));
    eth->eth_send(buf2, len, 1);

    clear_buffer();

    memset(&buf[0], 0, 0x1000); // Reset the buffer!
    current_size = 0;
  }
}

// change stdio putc pointer to our handler
void debug_server_enable_putc() {
  putc = &debug_server_putc;
}

// change stdio putc pointer to our handler
void debug_server_disable_putc() {
  putc = &no_putc;
}
