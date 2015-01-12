#ifndef __API_H__
#define __API_H__

#include <efi.h>
#include "efi/efi_82579LM.h"

#define API_ETH_TYPE 0xb00b

void test_send();
uint32_t send(const void *buf, uint32_t len, uint8_t flags);
uint32_t recv(void *buf, uint32_t len, uint8_t flags);
int uninstall(void);

extern EFI_HANDLE ih;
extern EFI_GUID guid_82579LM;
extern protocol_82579LM proto;

#endif

