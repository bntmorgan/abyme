#include <efi.h>
#include <efilib.h>

#include "stdio.h"
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

void debug_server_run(uint32_t exit_reason) {
  message_vmexit ms = {
    MESSAGE_VMEXIT,
    exit_reason
  };
  debug_server_send(&ms, sizeof(message_vmexit));
  uint8_t buf[0x100];
  message *mr = (message *)buf;
  mr->type = MESSAGE_MESSAGE;
  while (mr->type != MESSAGE_EXEC_CONTINUE) {
    if (debug_server_recv(mr, 0x100) == -1) {
      mr->type = MESSAGE_MESSAGE;
      continue;
    }
  }
}

void debug_server_send(void *buf, uint32_t len) {
  eth->send(buf, len, EFI_82579LM_API_BLOCK);
}

uint32_t debug_server_recv(void *buf, uint32_t len) {
  return eth->recv(buf, len, EFI_82579LM_API_BLOCK);
}
