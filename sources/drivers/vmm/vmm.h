/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __VMM_H__
#define __VMM_H__

#include <efi.h>

#include "types.h"

#define VMM_STACK_SIZE 0x400000
#define NB_EXIT_REASONS 65

enum VMM_VMCALL_IDS {
  VMM_VMCALL_NOPE,
  VMM_VMCALL_ENV,
  VMM_VMCALL_PERIOD
};

extern uint8_t vmm_stack[VMM_STACK_SIZE];

enum vm_exit_reason {
  EXIT_REASON_EXCEPTION_OR_NMI             = 0,
  EXIT_REASON_EXTERNAL_INTERRUPT           = 1,
  EXIT_REASON_TRIPLE_FAULT                 = 2,
  EXIT_REASON_INIT_SIGNAL                  = 3,
  EXIT_REASON_SIPI                         = 4,
  EXIT_REASON_IO_SMI                       = 5,
  EXIT_REASON_OTHER_SMI                    = 6,
  EXIT_REASON_INTR_WINDOW                  = 7,
  EXIT_REASON_NMI_WINDOW                   = 8,
  EXIT_REASON_TASK_SWITCH                  = 9,
  EXIT_REASON_CPUID                        = 10,
  EXIT_REASON_GETSEC                       = 11,
  EXIT_REASON_HLT                          = 12,
  EXIT_REASON_INVD                         = 13,
  EXIT_REASON_INVLPG                       = 14,
  EXIT_REASON_RDPMC                        = 15,
  EXIT_REASON_RDTSC                        = 16,
  EXIT_REASON_RSM                          = 17,
  EXIT_REASON_VMCALL                       = 18,
  EXIT_REASON_VMCLEAR                      = 19,
  EXIT_REASON_VMLAUNCH                     = 20,
  EXIT_REASON_VMPTRLD                      = 21,
  EXIT_REASON_VMPTRST                      = 22,
  EXIT_REASON_VMREAD                       = 23,
  EXIT_REASON_VMRESUME                     = 24,
  EXIT_REASON_VMWRITE                      = 25,
  EXIT_REASON_VMXOFF                       = 26,
  EXIT_REASON_VMXON                        = 27,
  EXIT_REASON_CR_ACCESS                    = 28,
  EXIT_REASON_MOV_DR                       = 29,
  EXIT_REASON_IO_INSTRUCTION               = 30,
  EXIT_REASON_RDMSR                        = 31,
  EXIT_REASON_WRMSR                        = 32,
  EXIT_REASON_INVALID_GUEST_STATE          = 33,
  EXIT_REASON_MSR_LOADING_FAILED           = 34,
  EXIT_REASON_MWAIT                        = 36,
  EXIT_REASON_MONITOR_TRAP_FLAG            = 37,
  EXIT_REASON_MONITOR                      = 39,
  EXIT_REASON_PAUSE                        = 40,
  EXIT_REASON_MCE_DURING_VM_ENTRY          = 41,
  EXIT_REASON_TPR_BELOW_THRESHOLD          = 43,
  EXIT_REASON_APIC_ACCESS                  = 44,
  EXIT_REASON_VIRTUALIZED_EOI              = 45,
  EXIT_REASON_ACCESS_GDTR_OR_IDTR          = 46,
  EXIT_REASON_ACCESS_LDTR_OR_TR            = 47,
  EXIT_REASON_EPT_VIOLATION                = 48,
  EXIT_REASON_EPT_MISCONFIG                = 49,
  EXIT_REASON_INVEPT                       = 50,
  EXIT_REASON_RDTSCP                       = 51,
  EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED = 52,
  EXIT_REASON_INVVPID                      = 53,
  EXIT_REASON_WBINVD                       = 54,
  EXIT_REASON_XSETBV                       = 55,
  EXIT_REASON_APIC_WRITE                   = 56,
  EXIT_REASON_RDRAND                       = 57,
  EXIT_REASON_INVPCID                      = 58,
  EXIT_REASON_VMFUNC                       = 59,
  EXIT_REASON_RDSEED                       = 61,
  EXIT_REASON_XSAVES                       = 63,
  EXIT_REASON_XRSTORS                      = 64,
};

struct registers {
  uint64_t rax;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rbx;
  uint64_t rsp;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t rip;
} __attribute__((packed));

void vmm_init(void);
void vmm_vm_exit_handler(void);
void vmm_set_period(uint64_t p);

#endif
