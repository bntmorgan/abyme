#ifndef __NESTED_VMX_H__
#define __NESTED_VMX_H__

#include "vmm.h"
#include "ept.h"

enum NESTED_STATE {
  NESTED_DISABLED,
  NESTED_HOST_RUNNING,
  NESTED_GUEST_RUNNING
};

extern uint8_t level;

void nested_recover_state(void);

void nested_vmxon(uint8_t *vmxon_guest);

void nested_vmclear(uint8_t *shadow_vmcs);

void nested_guest_vmptrld(uint8_t *shadow_vmcs, struct registers *gr);

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr);

void nested_vmlaunch(struct registers *guest_regs);

void nested_vmresume(struct registers *guest_regs);

void nested_vmxoff(struct registers *guest_regs);

uint64_t nested_vmread(uint64_t field);

void nested_vmwrite(uint64_t field, uint64_t value);

void nested_load_guest(void);

void nested_load_host(void);

void nested_vmx_shadow_bitmap_init(void);

void nested_interrupt_set(uint8_t vector, uint8_t type, uint32_t error_code);

void nested_interrupt_inject(void);

void nested_vmptrst(uint8_t **shadow_vmcs, struct registers *gr);

#endif
