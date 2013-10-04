#ifndef __API_H__
#define __API_H__

#define API_ETH_TYPE 0xb00b

uint32_t send(const void *buf, uint32_t len, uint8_t flags);
uint32_t recv(void *buf, uint32_t len, uint8_t flags);

#endif

