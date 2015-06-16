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
#include "cpuid.h"
#include "msr.h"
#include "cpu.h"
#include "level.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

struct nested_state ns = {
  NESTED_DISABLED,
  (uint8_t*)(uint64_t)-1,
  0,
  0,
  {0},
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

void nested_set_vm_succeed(void);

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

static int shadow_add(uint8_t *ptr, uint32_t *idx, uint64_t rip) {
  if (ns.guest_vmcs_idx >= GVMCS_NB) {
    return SHADOW_NO_SPACE;
  }
  ns.shadow_vmcs_ptr[ns.guest_vmcs_idx] = ptr;
  if (idx != NULL) {
    *idx = ns.guest_vmcs_idx;
  }
  ns.guest_vmcs_idx++;
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

// Get guest vmcs index from shadow
static int shadow_get_idx(uint8_t *ptr, uint32_t *idx) {
  uint32_t i;
  for (i = 0; i < ns.guest_vmcs_idx; i++) {
    if (ns.shadow_vmcs_ptr[i] == ptr) {
      if (idx != NULL) {
        *idx = i;
      }
      return SHADOW_OK;
    }
  }
  return SHADOW_NOT_FOUND;
}

// Get guest vmcs index from guest vmcs pointer
#ifdef _PARTIAL_VMX
static int guest_vmcs_get_idx(uint8_t *ptr, uint32_t *idx) {
  uint32_t i;
  for (i = 0; i < ns.guest_vmcs_idx; i++) {
    if ((uint8_t *)&guest_vmcs[i] == ptr) {
      if (idx != NULL) {
        *idx = i;
      }
      return SHADOW_OK;
    }
  }
  return SHADOW_NOT_FOUND;
}
#endif

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
  if (shadow_get_idx(ptr, &idx) == SHADOW_NOT_FOUND) {
    ERROR("VMCSs not found... (0x%016X)\n", ptr);
  }
  ns.shadow_idx = idx;
  // Set the level of virtualization
  ns.nested_level = idx + 2; // vmcs0 is already 1
  return SHADOW_OK;
}

/**
 * VMM state recovery in case of partial VMX
 */
#ifdef _PARTIAL_VMX
void nested_recover_state(void) {
  uint8_t *region = cpu_vmptrst();
  uint32_t shadow_idx;
  uint32_t nested_level = 0;
  uint8_t state = NESTED_DISABLED;
  // If the running guest is vmcs0
  if (ns.guest_vmx_operation == 0) {
    state = NESTED_DISABLED;
    nested_level = 1;
  } else if (region == &vmcs0[0]) {
    state = NESTED_HOST_RUNNING;
    nested_level = 1;
  } else {
    state = NESTED_GUEST_RUNNING;
    if(guest_vmcs_get_idx(region, &shadow_idx)) {
      ERROR("Guest VMCS not found...\n");
    }
    nested_level = shadow_idx + 2;
  }
  ns.state = state;
  ns.shadow_idx = shadow_idx;
  ns.nested_level = nested_level;
  if (state != ns.state || (state == NESTED_GUEST_RUNNING && shadow_idx !=
      ns.shadow_idx)) {
    ERROR("New information not corresponding...\n");
  }
  LEVEL(1, "state(0x%x), shadow_idx(0x%x), nested_level(0x%x)\n", ns.state,
      ns.shadow_idx, ns.nested_level);
}
#endif

/**
 * Event injection
 */ 

static uint8_t charged = 0;
static uint32_t error_code = 0;
static struct vm_entry_interrupt_info iif;

void nested_interrupt_set(uint8_t vector, uint8_t type, uint32_t error_code) {
  if (charged) {
    INFO("Multiple event injection unsupported : losing an interrupt \n");
  }
  charged = 1;
  iif.vector = vector;
  iif.type = type;
  if (error_code > 0 && (type == VM_ENTRY_INT_TYPE_SOFT_EXCEPTION || type ==
        VM_ENTRY_INT_TYPE_HW_EXCEPTION)) {
    iif.error_code = 1;
    error_code = error_code;
  }
  iif.valid = 1;
}

void nested_interrupt_inject(void) {
  if (charged) {
    charged = 0;
    cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, iif.raw);
    if (iif.error_code) {
      cpu_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, error_code);
    }
    INFO("Event Injection in 0x%x!\n", ns.nested_level);
  }
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
  // Notify the success
  nested_set_vm_succeed();

  // XXX ESXI
//   cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x8);
//   cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x4);
// 
//   INFO("IA32 TSC deadline 0x%016X : tsc 0x%016X\n",
//       msr_read(MSR_ADDRESS_IA32_TSC_DEADLINE), cpu_read_tsc());

  // check if vmcs addr is aligned on 4Ko and check rev identifier
  if (((uint64_t)vmxon_guest & 0xfff) != 0){
    panic("#!NESTED_VMXON addr is not 4k aligned\n");
    // (1 << 32) ignore if our VMCS is a shadow...
  } else if (((*(uint32_t*)vmxon_guest) & ~(1 << 31)) != (*(uint32_t*)vmcs0 & ~(1 << 31))) {
    panic("#!NESTED_VMXON rev identifier is not valid\n");
  }

  ns.state = NESTED_HOST_RUNNING;
  ns.guest_vmx_operation = 1;
}

void nested_vmclear(uint8_t *shadow_vmcs) {
  cpu_vmclear(shadow_vmcs);
  // set guest carry and zero flag to 0
  nested_set_vm_succeed();
  // cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

// No emulation, just keep the pointer
#ifdef _PARTIAL_VMX
void nested_guest_vmptrld(uint8_t *shadow_vmcs, struct registers *gr) {
  if (ns.state == NESTED_HOST_RUNNING) {
    ns.guest_shadow_vmcs_ptr[0] = shadow_vmcs;
  } else {
    ns.guest_shadow_vmcs_ptr[ns.shadow_idx + 1] = shadow_vmcs;
  }
}
#endif

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr) {

  // Remember the current shadow VMCS
  ns.shadow_ptr = shadow_vmcs;

  if (shadow_vmcs == 0x0) {
    ERROR("Shadow VMCS pointer is null\n");
  }
  // set guest carry and zero flag to 0
  nested_set_vm_succeed();
  // cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

#ifdef _VMCS_SHADOWING
  set_vmcs_link_pointer(shadow_vmcs);
#endif
}

void nested_vmptrst(uint8_t **shadow_vmcs, struct registers *gr) {

  // Give back the shadow ptr to the guest
  *shadow_vmcs = ns.shadow_ptr;

  // set guest carry and zero flag to 0
  // cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
  nested_set_vm_succeed();
}

#ifdef _NESTED_EPT
void nested_smap_build(void) {
  uint64_t start, end;
  uint64_t ta = 0;
  uint8_t in = 0;
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
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
        ept_perm(start, ((end - start) >> 12), 0x0, ns.shadow_idx + 1);
        in = 0;
      }
    }
    switch (s) {
      case PAGING_ENTRY_PTE:
        ta = a + 0x1000;
        break;
      case PAGING_ENTRY_PDE:
        ta = a + 0x200000;
        break;
      case PAGING_ENTRY_PDPTE:
        ta = a + 0x40000000;
        break;
      default:
        ERROR("BAD page size\n");
    }
    if (ta == ((uint64_t)1 << max_phyaddr)) {
      INFO("CHECK END maxphyaddr reached\n");
      return 0;
    } else {
      return 1;
    }
  }
  uint64_t eptp = ((cpu_vmread(EPT_POINTER_HIGH) << 32) |
    cpu_vmread(EPT_POINTER)) & ~((uint64_t)0xfff);
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
  // clear the RFLAGS.IF if this is a VMM

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
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif
  // Copy exec controls from vmcs0
  // uint32_t pinbased_ctls = cpu_vmread(PIN_BASED_VM_EXEC_CONTROL);
  nested_load_guest();
  // Handle all interrupts of the virtualized VMMs to reroute them into linux
  if (is_top_guest()) {
    // Inject protential previous event into the top guest
    nested_interrupt_inject();
  } else {
    // pinbased_ctls |= NMI_EXITING | EXT_INTR_EXITING;
  }
  // cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, pinbased_ctls);
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
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
  nested_smap_build();
#endif

  nested_shadow_to_guest();

  // Write the right VPID
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, ns.shadow_idx + 2); // vmcs0 is 1

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_GUEST_RUNNING;

  // Adjust vm entry controls
  INFO("rFAGZ 0x%016X\n", cpu_vmread(GUEST_RFLAGS));
  vmm_adjust_vm_entry_controls();

  // XXX
  // Adjust Unrestricted Guest
  cpu_vmptrld(ns.shadow_ptr);
  uint32_t procbased_ctls_2 = cpu_vmread(SECONDARY_VM_EXEC_CONTROL);
  uint32_t procbased_ctls = cpu_vmread(CPU_BASED_VM_EXEC_CONTROL);
  uint32_t pinbased_ctls = cpu_vmread(PIN_BASED_VM_EXEC_CONTROL);
  cpu_vmptrld(guest_vmcs[ns.shadow_idx]);
  INFO("Pinbased controls 0x%016X\n", pinbased_ctls);
  INFO("Procbased controls 0x%016X\n", procbased_ctls);
  INFO("Seconary procbased controls 0x%016X\n", procbased_ctls_2);
  if (procbased_ctls_2 & (uint32_t)UNRESTRICTED_GUEST) {
    INFO("Unrestricted guest\n");
  } else {
    INFO("Normal guest\n");
  }

  // XXX 

  nested_cpu_vmlaunch(guest_regs);
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in guest_vmcs instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    uint32_t idx;
    if (shadow_get_idx(ns.shadow_ptr, &idx) == SHADOW_NOT_FOUND) {
      INFO("vmread: shadow VMCS(0x%016X) is not launched, loading it\n");
    }
    cpu_vmptrld(guest_vmcs[idx]);
  } else {
    cpu_vmptrld(ns.shadow_ptr);
  }

  value = cpu_vmread(field);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  // cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
  nested_set_vm_succeed();

  return value;
}

void nested_vmwrite(uint64_t field, uint64_t value) {
  cpu_vmptrld(ns.shadow_ptr);
  cpu_vmwrite(field, value);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  // cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
  nested_set_vm_succeed();
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
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_HOST_RUNNING;
  ns.nested_level = 1;

  // handle Monitor trap flag
#ifdef _DEBUG_SERVER
  debug_server_mtf();
#endif
}

void nested_load_guest(void) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  nested_shadow_to_guest();

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  ns.state = NESTED_GUEST_RUNNING;

  // handle Monitor trap flag
#ifdef _DEBUG_SERVER
  debug_server_mtf();
#endif
}

#ifdef _PARTIAL_VMX
void nested_partial_vmresume(struct registers *guest_regs) {
  // Get the cnsh shadow ptr where we need to copy
  ns.shadow_ptr = ns.guest_shadow_vmcs_ptr[ns.shadow_idx + 1];

  INFO("SHADOW courante de l'hyperviseur 0x%02x : 0x%016X\n", ns.nested_level,
      ns.shadow_ptr);

  // XXX lol because of the effect of the partial vmx for resume we just need to
  // increment the shadow idx index
  ns.shadow_idx++;
  ns.nested_level = ns.shadow_idx + 2; // vmcs0 is already 1

  // Copy shadow 
  nested_load_guest();
  uint64_t grip  = cpu_vmread(GUEST_RIP);
  INFO("New rip 0x%016X\n", grip);

  // Change the current shadow
  ns.shadow_ptr = ns.shadow_vmcs_ptr[ns.shadow_idx];
  INFO("New shadow ptr 0x%016X\n", ns.shadow_ptr);

  // Set the mapping !
#ifdef _NESTED_EPT
  ept_set_ctx(ns.shadow_idx + 1); // 0 is for vmcs0
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  debug_server_panic(ns.state, 0, 0, guest_regs);
  // Let it go !
}
#endif

void nested_set_vm_succeed(void) {
  struct rflags rf;
  rf.raw = cpu_vmread(GUEST_RFLAGS);
  rf.cf = rf.pf = rf.af = rf.zf = rf.sf = rf.of = 0;
  cpu_vmwrite(GUEST_RFLAGS, rf.raw);
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
