#ifndef __ENV_MD5_H__
#define __ENV_MD5_H__

#include "vmm.h"

uint8_t env_md5_init(void);
uint8_t env_md5_call(struct registers *guest_regs);
uint8_t env_md5_execute(void);

#endif//__ENV_MD5_H__

