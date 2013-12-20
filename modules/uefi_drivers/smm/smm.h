#ifndef __SMM_H__
#define __SMM_H__

#include "types.h"

#define MSR_ADDRESS_IA32_SMBASE 0x92

uint64_t smm_get_smbase();
uint8_t smm_unlock_smram();

#endif//__SMM_H__
