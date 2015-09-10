#ifndef __VMM_SETUP_H__
#define __VMM_SETUP_H__

#include "idt.h"

struct setup_state {
  uint64_t protected_begin;
  uint64_t protected_end;
  uint64_t vm_RIP;
  uint64_t vm_RSP;
  uint64_t vm_RBP;
  uint64_t cr3;
  struct idt_ptr idt_ptr;
};

extern struct setup_state *setup_state;

void bsp_main(struct setup_state *state);

void vmm_setup(void);

void vmm_create_vmxon_region(void);

void vmm_vm_setup_and_launch();

#endif
