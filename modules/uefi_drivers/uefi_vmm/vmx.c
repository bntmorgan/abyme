#include "vmx.h"
#include "cpu.h"
#include "stdio.h"
#include "vmcs.h"
#include "msr.h"

extern uint64_t vm_RIP;
extern uint64_t vm_RSP;
extern uint64_t vm_RBP;

void cpu_enable_vmxe(void) {
  cpu_write_cr4(cpu_read_cr4() | 0x00002000);
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
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmptrld (%1) ;"
      "seta %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMPTRLD\n");
  }
}

void cpu_vmlaunch(void) {
  /* Set rsp and rip in VMCS here because they depend on the implementation of this function. */
  cpu_vmwrite(GUEST_RIP, vm_RIP);
  cpu_vmwrite(GUEST_RSP, vm_RSP);
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

__attribute__((sysv_abi)) void vmx_transition_display_error(uint8_t VMfailInvalid, uint8_t VMfailValid) {
  if (VMfailInvalid) {
    panic("#!VMX Transition VMfailInvalid\n");
  } else if(VMfailValid) {
    panic("#!VMX Transition VMfailValid, errcode=%d\n", cpu_vmread(VM_INSTRUCTION_ERROR));
  } else {
    panic("#!VMX Transition unkown error\n");
  }
}
