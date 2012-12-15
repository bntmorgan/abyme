#include "vmm.h"

#include "hardware/cpu.h"
#include "stdio.h"

void vmm_handle_vm_exit(gpr64_t guest_gpr) {
  guest_gpr.rsp = cpu_vmread(GUEST_RSP);
  uint32_t guest_rip = cpu_vmread(GUEST_RIP);
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);

  cpu_vmwrite(GUEST_RIP, guest_rip + exit_instruction_length);

  INFO("VMEXIT! exit_reason = %d\n", exit_reason);
  INFO("----------\n");
  INFO("rip = 0x%x\n", guest_rip);
  INFO("rsp = 0x%x    rbp = 0x%x\n", guest_gpr.rsp, guest_gpr.rbp);
  INFO("rax = 0x%x    rbx = 0x%x\n", guest_gpr.rax, guest_gpr.rbx);
  INFO("rcx = 0x%x    rdx = 0x%x\n", guest_gpr.rcx, guest_gpr.rdx);
  INFO("rsi = 0x%x    rdi = 0x%x\n", guest_gpr.rsi, guest_gpr.rdi);
  INFO("r8  = 0x%x    r9  = 0x%x\n", guest_gpr.r8,  guest_gpr.r9);
  INFO("r10 = 0x%x    r11 = 0x%x\n", guest_gpr.r10, guest_gpr.r11);
  INFO("r12 = 0x%x    r13 = 0x%x\n", guest_gpr.r12, guest_gpr.r13);
  INFO("r14 = 0x%x    r15 = 0x%x\n", guest_gpr.r14, guest_gpr.r15);

  switch (exit_reason) {
    case EXIT_REASON_CPUID:
      INFO("handling CPUID (rax = %x)\n", guest_gpr.rax);
      uint64_t command = guest_gpr.rax;
      __asm__ __volatile__("cpuid" : "=a" (guest_gpr.rax),
          "=b" (guest_gpr.rbx), "=c" (guest_gpr.rcx),
          "=d" (guest_gpr.rdx) : "a" (guest_gpr.rax));
      if (command == 0) {
        // 3p1cW1n!\o/!
        guest_gpr.rbx = 0x63317033;
        guest_gpr.rdx = 0x216e3157;
        guest_gpr.rcx = 0x212f6f5c;
      }
      break;
    case EXIT_REASON_RDMSR:
      INFO("handling RDMSR (rcx = %x)\n", guest_gpr.rcx);
      __asm__ __volatile__("rdmsr" : "=a" (guest_gpr.rax), "=d" (guest_gpr.rdx) : "c" (guest_gpr.rcx));
      break;
    case EXIT_REASON_WRMSR:
      INFO("handling WRMSR (rcx = %x, rax = %x, rdx = %x)\n", guest_gpr.rcx, guest_gpr.rax, guest_gpr.rdx);
      __asm__ __volatile__("wrmsr" : : "a" (guest_gpr.rax), "d" (guest_gpr.rdx), "c" (guest_gpr.rcx));
      break;
    case EXIT_REASON_TASK_SWITCH: {
      // Checks here a gneral protection VM_EXIT
      // 25.4.2 Treatment of Task Switches
      // If CALL, INT n, or JMP accesses a task gate in IA-32e mode, a general-protection exception occurs
      // BIOS Call INT 15h (rax a e820)
      // Get the vm exit interrupt information
      uint32_t int_info = cpu_vmread(VM_EXIT_INTR_INFO);
      // interruption 0x15 Miscellaneous system services
      if ((int_info & 0xff) == 0x15 && ((int_info & 0x700) >> 8) == 0x6) {
        // Query System Address Map gate e820
        if ((guest_gpr.rax & 0xff) == 0xe820) {
          INFO("BIOS interrupt call 0xe820\n");
          while(1);
        }
      }
    }
    default:
      INFO("unhandled reason: %d\n", exit_reason);
      BREAKPOINT();
  }
}
