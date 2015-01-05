#ifndef __ENV_MD5_H__
#define __ENV_MD5_H__

#include "vmm.h"

enum ENV_MD5_VMCALL_IDS {
  ENV_MD5_VMCALL_ADDR,
  ENV_MD5_VMCALL_FLIP,
  ENV_MD5_VMCALL_UNFLIP,
};

int env_md5_init(void);
int env_md5_call(struct registers *guest_regs);
int env_md5_execute(void);

#endif//__ENV_MD5_H__

