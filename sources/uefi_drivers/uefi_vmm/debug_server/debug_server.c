#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "string.h"
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
  uint8_t *b = (uint8_t *)(mr++);
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

void debug_server_run(uint32_t exit_reason) {
  message_vmexit ms = {
    MESSAGE_VMEXIT,
    debug_server_get_core(),
    exit_reason
  };
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
        case MESSAGE_MEMORY_READ: {
          debug_server_handle_memory_read((message_memory_read*)mr);
        }
        case MESSAGE_MEMORY_WRITE: {
          debug_server_handle_memory_write((message_memory_write*)mr);
        }
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
