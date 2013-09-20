#include <efi.h>
#include <efilib.h>

#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2        0x48B

uint64_t msr_read(uint64_t msr_address) {
  uint32_t eax, edx;
  __asm__ __volatile__("rdmsr" : "=a" (eax), "=d" (edx) : "c" (msr_address));
  return (((uint64_t) edx) << 32) | ((uint64_t) eax);
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

/**
 * The VMCS link pointer field in the current VMCS (see Section 24.4.2) is
 * itself the address of a VMCS. If VM entry
 * is performed successfully with the 1-setting of the “VMCS shadowing”
 * VM-execution control, the VMCS
 * referenced by the VMCS link pointer field becomes active on the logical
 * processor. The identity of the current
 * VMCS does not change.
 */
EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  // Print(L"VMCS shadowing supported ? :%d\n", !(cpu_adjust32((1 << 14), MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2) == 0));
  int thunes = cpu_adjust32((1 << 14), MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2) & (1 << 14);
  if(thunes) {
    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"VMCS SHADOWING !!!\n");
  } else {
    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"No VMCS Shadowning :'(\n");
  }
  
  if (msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2)) {
    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"MSR non zéro\n");
  } else {
    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"MSR null no yolol\n");
  }

  return EFI_SUCCESS;
}
