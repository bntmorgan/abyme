#include "vmx.h"
#include "cpu.h"
#include "stdio.h"
#include "vmcs.h"
#include "msr.h"

void cpu_enable_vmxe(void) {
  cpu_write_cr4(cpu_read_cr4() | 0x00002000);
}

void cpu_vmxoff(void) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmxoff;"
      "setae %%cl ;"
    : "=c" (ok));
  if (ok == 0x0) {
    panic("!#CPU VMXOFF\n");
  }
}

void cpu_vmxon(uint8_t *region) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmxon (%1) ;"
      "setae %%cl ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMXON\n");
  }
}

void cpu_vmclear(uint8_t *region) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmclear (%1) ;"
      "seta %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMCLEAR\n");
  }
}

void cpu_vmptrld(uint8_t *region) {
  uint16_t ok = 0;
  __asm__ __volatile__(
      "vmptrld (%1) ;"
      "setc %%cl    ;"
      "setz %%ch    ;"
    : "=c" (ok) : "D" (&region));
  if (ok != 0x0) {
    // panic("!#CPU VMPTRLD\n");
    INFO("REGION 0x%016X\n", region);
    vmx_transition_display_error((ok >> 8) & 0xff, (ok >> 0) & 0xff);
  }
}

uint8_t *cpu_vmptrst(void) {
  uint16_t ok = 0;
  uint8_t *region;
  __asm__ __volatile__(
      "vmptrst (%1) ;"
      "setc %%cl    ;"
      "setz %%ch    ;"
    : "=c" (ok) : "D" (&region));
  if (ok != 0x0) {
    // panic("!#CPU VMPTRLD\n");
    INFO("REGION 0x%016X\n", region);
    vmx_transition_display_error((ok >> 8) & 0xff, (ok >> 0) & 0xff);
  }
  return region;
}

void cpu_vmlaunch(uint64_t vm_RIP, uint64_t vm_RSP, uint64_t vm_RBP) {
  /* Set rsp and rip in VMCS here because they depend on the implementation of this function. */
  cpu_vmwrite(GUEST_RIP, vm_RIP);
  cpu_vmwrite(GUEST_RSP, vm_RSP);
  INFO("rip 0x%016X, rsp 0x%016x, rbp 0x%016X\n", vm_RIP, vm_RSP, vm_RBP);
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (vm_RBP));
	/* Launch the vm */
  __asm__ __volatile__("vmlaunch;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void cpu_vmwrite(uint64_t field, uint64_t value) {
  __asm__ __volatile__("vmwrite %%rax, %%rdx" : : "a" (value), "d" (field));
}

uint64_t cpu_vmread(uint64_t field) {
  uint64_t value;
  __asm__ __volatile__("vmread %%rdx, %%rax" : "=a" (value): "d" (field));
  return value;
}

uint32_t cpu_adjust32(uint32_t value, uint32_t msr_address) {
  uint64_t msr_value = msr_read(msr_address);
  value |= (msr_value >>  0) & 0xffffffff;
  value &= (msr_value >> 32) & 0xffffffff;
  return value;
}

uint64_t cpu_adjust64(uint64_t value, uint32_t msr_fixed0, uint32_t msr_fixed1) {
  return (value & msr_read(msr_fixed1)) | msr_read(msr_fixed0);
}

uint8_t cpu_vmread_safe(unsigned long field, unsigned long *value)
{
  uint8_t okay;
  __asm__ __volatile__ (
                 "vmread %2, %1\n\t"
                 /* CF==1 or ZF==1 --> rc = 0 */
                 "setnbe %0"
                 : "=qm" (okay), "=rm" (*value)
                 : "r" (field));
  return okay;
}

// From linux
#define ASM_VMX_INVVPID     ".byte 0x66, 0x0f, 0x38, 0x81, 0x08"
void cpu_invvpid(int type, uint16_t vpid, uint64_t gva) {
  struct {
    uint64_t vpid : 16;
    uint64_t rsvd : 48;
    uint64_t gva;
  } operand = { vpid, 0, gva };

  asm volatile (ASM_VMX_INVVPID
      /* CF==1 or ZF==1 --> rc = -1 */
      "; ja 1f ; ud2 ; 1:"
      : : "a"(&operand), "c"(type) : "cc", "memory");
}

__attribute__((sysv_abi)) void vmx_transition_display_error(uint8_t VMfailInvalid, uint8_t VMfailValid) {
  if (VMfailInvalid) {
    panic("#!VMX Transition VMfailInvalid\n");
  } else if(VMfailValid) {
    // Dump the entire VMCS
    // XXX dump vmcs
//    vmcs_update();
//    vmcs_dump(vmcs);
    uint8_t e = cpu_vmread(VM_INSTRUCTION_ERROR);
    printk("#!VMX Transition VMfailValid, errcode=%d\n", e);
    switch (e) {
      case 1:
        panic("VMCALL executed in VMX root operation\n");
      case 2:
        panic("VMCLEAR with invalid physical address\n");
      case 3:
        panic("VMCLEAR with VMXON pointer\n");
      case 4:
        panic("VMLAUNCH with non-clear VMCS\n");
      case 5:
        panic("VMRESUME with non-launched VMCS\n");
      case 6:
        panic("VMRESUME after VMXOFF (VMXOFF and VMXON between VMLAUNCH and VMRESUME)\n");
      case 7:
        panic("VM entry with invalid control field(s)\n");
      case 8:
        panic("VM entry with invalid host-state field(s)\n");
      case 9:
        panic("VMPTRLD with invalid physical address\n");
      case 10:
        panic("VMPTRLD with VMXON pointer\n");
      case 11:
        panic("VMPTRLD with incorrect VMCS revision identifier\n");
      case 12:
        panic("VMREAD/VMWRITE from/to unsupported VMCS component\n");
      case 13:
        panic("VMWRITE to read-only VMCS component\n");
      case 15:
        panic("VMXON executed in VMX root operation\n");
      case 16:
        panic("VM entry with invalid executive-VMCS pointer\n");
      case 17:
        panic("VM entry with non-launched executive VMCS\n");
      case 18:
        panic("VM entry with executive-VMCS pointer not VMXON pointer (when attempting to deactivate the dual-monitor treatment of SMIs and SMM)\n");
      case 19:
        panic("VMCALL with non-clear VMCS (when attempting to activate the dual-monitor treatment of SMIs and SMM)\n");
      case 20:
        panic("VMCALL with invalid VM-exit control fields\n");
      case 22:
        panic("VMCALL with incorrect MSEG revision identifier (when attempting to activate the dual-monitor treatment of SMIs and SMM)\n");
      case 23:
        panic("VMXOFF under dual-monitor treatment of SMIs and SMM\n");
      case 24:
        panic("VMCALL with invalid SMM-monitor features (when attempting to activate the dual-monitor treatment of SMIs and SMM)\n");
      case 25:
        panic("VM entry with invalid VM-execution control fields in executive VMCS (when attempting to return from SMM)\n");
      case 26:
        panic("VM entry with events blocked by MOV SS\n");
      case 28:
        panic("Invalid operand to INVEPT/INVVPID.\n");
      default:
        panic("Bad error code\n");
    }
  } else {
    panic("#!VMX Transition unkown error\n");
  }
}
