#ifndef __FLASH_H__
#define __FLASH_H__

int env_flash_init(void);
int env_flash_call(struct registers *guest_regs);
int env_flash_execute(void);

#endif//__FLASH_H__
