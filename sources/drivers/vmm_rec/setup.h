#ifndef __VMM_SETUP_H__
#define __VMM_SETUP_H__

struct setup_state {
  uint64_t protected_begin;
  uint64_t protected_end;
  uint64_t vm_RIP;
  uint64_t vm_RSP;
  uint64_t vm_RBP;
};

void bsp_main(struct setup_state *state);

void vmm_setup(void);

void vmm_create_vmxon_and_vmcs_regions(void);

void vmm_vm_setup_and_launch(struct setup_state *state);

#endif
