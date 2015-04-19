#include <efi.h>
#include <efilib.h>
#include "vmm.h"
#include "nested_vmx.h"
#include "vmcs.h"
#include "vmx.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "nested_vmx.h"
#include "paging.h"
#include "ept.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

struct nested_state ns = {
  NESTED_DISABLED,
  (uint8_t*)(uint64_t)-1,
  0,
  0,
  {0}
};

static uint64_t fields_values[NB_VMCS_FIELDS];
#define READ_VMCS_FIELDS(fields) \
  read_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))
#define WRITE_VMCS_FIELDS(fields) \
  write_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))

static inline void read_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);
static inline void write_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);

#ifdef _VMCS_SHADOWING
uint8_t vmread_bitmap [4096] __attribute__((aligned(0x1000)));
uint8_t vmwrite_bitmap[4096] __attribute__((aligned(0x1000)));
#endif

enum shadow_err_code {
  SHADOW_OK,
  SHADOW_NO_SPACE,
  SHADOW_NOT_FOUND
};

/**
 * Gestion des VMCS GUEST
 */
uint8_t guest_vmcs[GVMCS_NB][4096] __attribute((aligned(0x1000)));
struct nested_state *gns[GVMCS_NB];
static uint8_t guest_vmcs_idx = 0;

static int shadow_add(uint8_t *ptr, uint32_t *idx, uint64_t rip) {
  if (guest_vmcs_idx >= GVMCS_NB) {
    return SHADOW_NO_SPACE;
  }
  ns.shadow_vmcs_ptr[guest_vmcs_idx] = ptr;
  if (idx != NULL) {
    *idx = guest_vmcs_idx;
  }
  guest_vmcs_idx++;
  return SHADOW_OK;
}

#ifdef _VMCS_SHADOWING
static void shadow_bitmap_read_set_field(uint64_t field) {
  vmread_bitmap[(field & 0x7fff) >> 3] |= (1 << ((field & 0x7fff) & 0x7));
}

void nested_vmx_shadow_bitmap_init(void) {
  static uint64_t fields[] = {NESTED_READ_ONLY_DATA_FIELDS};
  memset(&vmwrite_bitmap[0], 0, 4096);
  memset(&vmread_bitmap[0], 0, 4096);
  uint32_t i;
  for (i = 0; i < sizeof(fields) / sizeof(uint64_t); i++) {
    shadow_bitmap_read_set_field(fields[i]);
  }
}
#endif

static int shadow_init(uint32_t idx) {
  // init guest VMCS
  memset((uint8_t *) &guest_vmcs[idx][0], 0, 0x1000);
  *(uint32_t*)&guest_vmcs[idx][0] = *(uint32_t*)&vmcs0[0];
  cpu_vmclear(&guest_vmcs[idx][0]);
  return SHADOW_OK;
}

static int shadow_get_do(uint8_t *ptr, uint32_t *idx, uint8_t **tab) {
  uint32_t i;
  for (i = 0; i < guest_vmcs_idx; i++) {
    if (tab[i] == ptr) {
      if (idx != NULL) {
        *idx = i;
      }
      return SHADOW_OK;
    }
  }
  return SHADOW_NOT_FOUND;
}

static int shadow_get(uint8_t *ptr, uint32_t *idx) {
 return shadow_get_do(ptr, idx, &ns.shadow_vmcs_ptr[0]);
}

static int shadow_new(uint8_t *ptr, uint64_t rip) {
  uint32_t idx;
  if (!shadow_add(ptr, &idx, rip)) {
    shadow_init(idx);
  } else {
    ERROR("No more space for VMCS\n");
  }
  ns.shadow_idx = idx;
  // Set the level of virtualization
  ns.nested_level = idx + 2; // vmcs0 is already 1
  return SHADOW_OK;
}

static int shadow_set(uint8_t *ptr) {
  uint32_t idx;
  if (shadow_get(ptr, &idx) == SHADOW_NOT_FOUND) {
    ERROR("VMCSs not found...\n");
  }
  ns.shadow_idx = idx;
  // Set the level of virtualization
  ns.nested_level = idx + 2; // vmcs0 is already 1
  return SHADOW_OK;
}

/**
 * VMX Emulation
 */

#ifdef _VMCS_SHADOWING
static void set_vmcs_link_pointer(uint8_t *shadow_vmcs) {
  // Set the VMCS link pointer for vmcs0
  cpu_vmwrite(VMCS_LINK_POINTER, ((uint64_t)shadow_vmcs) & 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, (((uint64_t)shadow_vmcs) >> 32) & 0xffffffff);
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL) |
      VMCS_SHADOWING);
  // Exit bitmap
  cpu_vmwrite(VMREAD_BITMAP_ADDR, (uint64_t)&vmread_bitmap & 0xffffffff);
  cpu_vmwrite(VMREAD_BITMAP_ADDR_HIGH, ((uint64_t)&vmread_bitmap >> 32) & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR, (uint64_t)&vmwrite_bitmap & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR_HIGH, ((uint64_t)&vmwrite_bitmap >> 32) & 0xffffffff);
  // Set the type of the shadow_vmcs
  *(uint32_t *)shadow_vmcs |= (1 << 31);
}
#endif

void nested_vmxon(uint8_t *vmxon_guest) {
  if (ns.state != NESTED_DISABLED) {
    panic("#!NESTED_VMXON not disabled\n");
  }
  // set guest carry flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x1);

  // check if vmcs addr is aligned on 4Ko and check rev identifier
  if (((uint64_t)vmxon_guest & 0xfff) != 0){
    panic("#!NESTED_VMXON addr is not 4k aligned\n");
    // (1 << 32) ignore if our VMCS is a shadow...
  } else if (((*(uint32_t*)vmxon_guest) & ~(1 << 31)) != (*(uint32_t*)vmcs0 & ~(1 << 31))) {
    panic("#!NESTED_VMXON rev identifier is not valid\n");
  }

  ns.state = NESTED_HOST_RUNNING;
}

void nested_vmclear(uint8_t *shadow_vmcs) {
  cpu_vmclear(shadow_vmcs);
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr) {

  // Remember the current shadow VMCS
  ns.shadow_ptr = shadow_vmcs;

  if (shadow_vmcs == 0x0) {
    ERROR("Shadow VMCS pointer is null\n");
  }
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

#ifdef _VMCS_SHADOWING
  set_vmcs_link_pointer(shadow_vmcs);
#endif
}

#ifdef _NESTED_EPT
void nested_smap_build(void) {
  uint64_t start, end;
  uint8_t in = 0;
  // XXX already done in shadow to guest
  cpu_vmptrld(ns.shadow_ptr);
  // Private callback for chunk detection
  int cb(uint64_t *e, uint64_t a, uint8_t s) {
    if (in == 0) {
      if ((*e & 0x7) == 0) {
        start = a;
        in = 1;
      }
    } else {
      if ((*e & 0x7) != 0) {
        end = a; 
        // Add the detected memory chunk to the smap
        INFO("chunk(0x%x, 0x%016X, 0x%016X)\n", ns.shadow_idx, start, (end -
              start) >> 12);
        in = 0;
      }
    }
    return 1;
  }
  uint64_t eptp = (cpu_vmread(EPT_POINTER_HIGH) << 32) |
    cpu_vmread(EPT_POINTER);
  INFO("EPTP 0x%016X\n", eptp);
  if (ept_iterate(eptp, &cb)) {
    ERROR("PAGING error... 0x%x\n", paging_error);
  }
}
#endif

void nested_shadow_to_guest(void) {
  static uint64_t guest_fields[] = { NESTED_COPY_FROM_SHADOW };
  cpu_vmptrld(ns.shadow_ptr);
  READ_VMCS_FIELDS(guest_fields);
#ifdef _VMCS_SHADOWING
  // !!! With VMCS shadowing write exits are bypassed !!!
  // Copy the Shadowing configuration from the shadow
  uint64_t sec_ctrl = cpu_vmread(SECONDARY_VM_EXEC_CONTROL) & VMCS_SHADOWING;
  uint64_t bit31 = *(uint32_t *)ns.shadow_ptr & (1 << 31);
#endif
  cpu_vmptrld(guest_vmcs[ns.shadow_idx]);
  WRITE_VMCS_FIELDS(guest_fields);
#ifdef _VMCS_SHADOWING
  // Activate VMCS shadowing if any
  if (sec_ctrl) {
    cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL)
        | VMCS_SHADOWING);
  } else {
    cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL)
        & ~VMCS_SHADOWING);
  }
  // Set the type of the shadow_vmcs
  if (bit31) {
    *(uint32_t *)ns.shadow_ptr |= (1 << 31);
  } else {
    *(uint32_t *)ns.shadow_ptr &= ~(1 << 31);
  }
#endif
}

void nested_cpu_vmresume(struct registers *guest_regs) {
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (guest_regs->rbp));
  /* Launch the vm */
  __asm__ __volatile__("vmresume;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void nested_cpu_vmlaunch(struct registers *guest_regs) {
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (guest_regs->rbp));
  /* Launch the vm */
  __asm__ __volatile__("vmlaunch;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void nested_vmresume(struct registers *guest_regs) {
  // Current shadow VMCS will be executed !
  shadow_set(ns.shadow_ptr);
#ifdef _NESTED_EPT
  ept_set_ctx(ns.shadow_idx + 1); // 0 is for vmcs0
  uint64_t desc1[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type1 = 0x2;
  // Flush all tlb caches #YOLO
  __asm__ __volatile__("invvpid %0, %1" : : "m"(desc1), "r"(type1));
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x1;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif
  nested_load_guest();
}

void nested_vmlaunch(struct registers *guest_regs) {
  static uint64_t ctrl_host_fields[] = { NESTED_CTRL_FIELDS, NESTED_HOST_FIELDS };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // Current shadow VMCS will be really executed, we allocate a VMCS for it
  shadow_new(ns.shadow_ptr, guest_regs->rip);

  // copy ctrl fields + host fields from vmcs0 to guest_vmcs
  READ_VMCS_FIELDS(ctrl_host_fields);
  cpu_vmptrld(guest_vmcs[ns.shadow_idx]);
  WRITE_VMCS_FIELDS(ctrl_host_fields);

#ifdef _NESTED_EPT
  ept_set_ctx(ns.shadow_idx + 1); // 0 is for vmcs0
  uint64_t desc1[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type1 = 0x2;
  // Flush all tlb caches #YOLO
  __asm__ __volatile__("invvpid %0, %1" : : "m"(desc1), "r"(type1));
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x1;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
//   nested_smap_build();
#endif

  nested_shadow_to_guest();

  // Write the right VPID
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, ns.shadow_idx + 2); // vmcs0 is 1

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_GUEST_RUNNING;

  nested_cpu_vmlaunch(guest_regs);
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in guest_vmcs instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    uint32_t idx;
    if (shadow_get(ns.shadow_ptr, &idx) == SHADOW_NOT_FOUND) {
      ERROR("VMCSs not found...\n");
    }
    cpu_vmptrld(guest_vmcs[idx]);
  } else {
    cpu_vmptrld(ns.shadow_ptr);
  }

  value = cpu_vmread(field);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

  return value;
}

void nested_vmwrite(uint64_t field, uint64_t value) {
  cpu_vmptrld(ns.shadow_ptr);
  cpu_vmwrite(field, value);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_load_host(void) {
  static uint64_t guest_fields[] = { NESTED_GUEST_FIELDS, VM_ENTRY_CONTROLS /* For IA32E_MODE_GUEST */ };
  static uint64_t host_fields[] = { NESTED_HOST_FIELDS };
  static uint64_t host_fields_dest[] = { NESTED_HOST_FIELDS_DEST };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy all fields updated by vm_exit from guest_vmcs to shadow_vmcs
  READ_VMCS_FIELDS(guest_fields);
  cpu_vmptrld(ns.shadow_ptr);
  WRITE_VMCS_FIELDS(guest_fields);

  // restore host fields from shadow_vmcs to vmcs0
  READ_VMCS_FIELDS(host_fields);
  cpu_vmptrld(vmcs0);
  WRITE_VMCS_FIELDS(host_fields_dest);

#ifdef _NESTED_EPT
  // Restore ept mapping
  ept_set_ctx(0); // 0 is for vmcs0
  uint64_t desc1[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type1 = 0x2;
  // Flush all tlb caches #YOLO
  __asm__ __volatile__("invvpid %0, %1" : : "m"(desc1), "r"(type1));
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x1;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_HOST_RUNNING;
  ns.nested_level = 1;
}

void nested_load_guest(void) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  nested_shadow_to_guest();

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_GUEST_RUNNING;
}

static inline void read_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields) {
  int i;
  for (i = 0; i < nb_fields ; i++) {
    values[i] = cpu_vmread(fields[i]);
  }
}

static inline void write_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields) {
  int i;
  for (i = 0; i < nb_fields ; i++) {
    cpu_vmwrite(fields[i], values[i]);
  }
}
