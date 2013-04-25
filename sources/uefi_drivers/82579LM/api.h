#ifndef __API_H__
#define __API_H__

#define API_BLOCK (1 << 0)

uint32_t send(const void *buf, uint32_t len, uint8_t flags);
uint32_t revc(void *buf, uint32_t len, uint8_t flags);

#endif

