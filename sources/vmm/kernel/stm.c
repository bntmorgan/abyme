/**
 * SMM Transfert Monitor 
 */
#include "hardware/msr.h"
#include "vmm.h"

int stm_check_dual_monitor() {
  uint64_t val = msr_read64(MSR_ADDRESS_IA32_VMX_BASIC);
  return ((val << 49) && 1);
}
