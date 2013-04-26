#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "debug_server.h"
#include "efi/efi_82579LM.h"
  
protocol_82579LM *eth;

void debug_server_init() {
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID; 

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }
}

void debug_server_send(void *buf, uint32_t len) {
  eth->send(buf, len, 0);
}
