/**
 * SMM Transfert Monitor 
 */
#include "hardware/msr.h"
#include "vmm.h"

int stm_check_dual_monitor() {
  uint64_t val = msr_read64(MSR_ADDRESS_IA32_VMX_BASIC);
  return ((val << 49) && 1);
}

void stm_enable_monitor() {
  uint32_t l = 0x9001, h = 0; // Intel programming manual section 3.15.5
  msr_write(MSR_ADDRESS_IA32_SMM_MONITOR_CTL, l, h);
}
