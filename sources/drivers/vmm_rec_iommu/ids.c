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

#include "stdio.h"
#include "hook.h"
#include "vmm.h"
#include "vmcs.h"
#include "ept.h"
#include "dmar.h"
#include "pci.h"
#include "string.h"
#include "efiw.h"
#include "efi/efi_eric.h"
#include "debug.h"

protocol_eric *eric;

enum iommu_trap {
  IOMMU_RUNTIME,
  IOMMU_TRAP
};

enum iommu_trap iommu_state = IOMMU_RUNTIME;

static uint64_t guest_physical_addr;

void iommu_protect(void) {
  ept_perm((uint64_t)vtd1, 1, 5, 0);
  ept_perm((uint64_t)vtd2, 1, 5, 0);
  ept_inv_tlb();
}

void iommu_unprotect(void) {
  ept_perm((uint64_t)vtd1, 1, 7, 0);
  ept_perm((uint64_t)vtd2, 1, 7, 0);
  ept_inv_tlb();
}

int iommu_boot_ept(struct registers *regs) {
  VMR(info.guest_physical_address);
  guest_physical_addr = vmcs->info.guest_physical_address.raw;
  INFO("EPT VIOLATION @0x%016X\n", guest_physical_addr);
  if ((guest_physical_addr & ~(uint64_t)0xfff) == vtd1 ||
      (guest_physical_addr & ~(uint64_t)0xfff) == vtd2 ) {
    iommu_state = IOMMU_TRAP;
    vmm_mtf_set();
    iommu_unprotect();
    // Pre access operations
    switch (guest_physical_addr & 0xfff) {
      case PCI_VC0PREMAP_GCMD:
        INFO("=========== Write to GCMD\n");
        if ((guest_physical_addr & ~(uint64_t)0xfff) == vtd2) {
          dmar_dump_iommu(vtd2);
          union vtd_rtaddr rtaddr = {.raw = pci_bar_readq(vtd2,
              PCI_VC0PREMAP_RTADDR)};
          // Get root table
          if (rtaddr.raw == 0) {
              INFO("RTADDR is zero !\n");
          } else {
            dmar_dump(rtaddr);
            union dmar_root_entry *rt = (union dmar_root_entry *)rtaddr.raw;
            // Get bus 0 context table
            if (rt[0].p == 0) {
              INFO("RT[0] not present !\n");
            } else {
              union dmar_context_entry *ct =
                (union dmar_context_entry *)(uintptr_t)(rt[0].ctp << 12);
              // Unprotect e1000 NIC [00:19.0], entry 0x19 * 8
              INFO("Unprotecting [00:19.0] network card\n");
              ct[0x19 << 3].p = 1;
              ct[0x19 << 3].t = 2;
              ct[0x19 << 3].aw = 2;
              ct[0x19 << 3].did = 2;
              __asm__ __volatile__("wbinvd");
              __asm__ __volatile__("wbinvd");
              __asm__ __volatile__("wbinvd");
              __asm__ __volatile__("wbinvd");
              dmar_dump(rtaddr);
            }
          }
        }
        break;
    }
    INFO("STATE -> TRAP !\n");
    return HOOK_OVERRIDE_STAY;
  } else {
    return HOOK_OVERRIDE_SKIP;
  }
}

int iommu_boot_mtf(struct registers *regs) {
  if (iommu_state == IOMMU_TRAP) {
    // Post access operations
    switch (guest_physical_addr & 0xfff) {
      case PCI_VC0PREMAP_GCMD:
        INFO("=========== Write to GCMD\n");
        break;
      case PCI_VC0PREMAP_RTADDR:
        INFO("=========== Write to RTADDR\n");
        break;
//       case PCI_VC0PREMAP_IQT:
//         INFO("=========== Write to IQT\n");
//         break;
    }
    // print_protected_memory();
    INFO("IOMMU TRAPPED !\n");
    iommu_protect();
    vmm_mtf_unset();
    iommu_state = IOMMU_RUNTIME;
    INFO("STATE -> RUNTIME !\n");
  }
  return 0;
}

union dmar_context_entry *fce;

int iommu_boot_vmcall2(struct registers *regs) {
  dmar_dump_iommu(vtd2);
  union vtd_rtaddr rtaddr = {.raw = pci_bar_readq(vtd2, PCI_VC0PREMAP_RTADDR)};
  dmar_dump(rtaddr);
  return 0;
}

int iommu_boot_vmcall(struct registers *regs) {
  INFO("IOMMU address (@0x%016X)\n", regs->rax);
  uint64_t iommu = regs->rax;
  if (iommu != vtd2) {
    return 0;
  }
  union dmar_root_entry fre;
  // get the rtaddr
  union vtd_rtaddr rtaddr = {.raw = pci_bar_readq(iommu, PCI_VC0PREMAP_RTADDR)};
  dmar_dump(rtaddr);
  // Preparing fake context entry for devil device
  // Context entry for pass through
  memset(&fce[0], 0, sizeof(union dmar_context_entry) * 256);
  fce[0].p = 1;
  fce[0].t = 2; // No translation ! haha CAP.PT is 1
  fce[0].aw = 2; // 48 address width

  // Root Entry for bus 1 device 0 function 0
  memset(&fre, 0, sizeof(union dmar_root_entry));
  fre.p = 1;
  fre.ctp = (((uint64_t)&fce[0]) >> 12);

  // fre.ctp = ((uint64_t)eric->bar0 >> 12);
  // dump(eric->bar0, 2, 0x10, 8, (uint32_t)(uintptr_t)eric->bar0, 2);
  // uint64_t *lol = (uint64_t*)eric->bar0;
  // printk("LOL access 64 bites (0x%016X)\n", *lol);

  INFO("Fake root entry to write for pass through\n");
  dmar_print_dmar_root_entry(fre);
  INFO("Fake context entry to write for pass through\n");
  dmar_print_dmar_context_entry(fce[0]);
  uint64_t entry_eric, entry_nic;
  // Root entry[1] address
  entry_eric = ((uint64_t)rtaddr.rta << 12) + sizeof(union dmar_root_entry);
  INFO("Address of bus 1 root entry (@0x%016X)\n", entry_eric);
  union dmar_root_entry re = {.raw = {*(uint64_t*)((uint64_t)rtaddr.rta << 12),
    0}};
  entry_nic = ((uint64_t)re.ctp << 12) + sizeof(union dmar_context_entry) *
    0x19 * 0x8; // Address of our nic
  INFO("Address of bus 0 context entry dev 19 fun 0 (@0x%016X)\n", entry_nic);
  INFO("Writing!\n");
//   *(uint64_t*)entry_eric = fre.raw[0];
//   *(uint64_t*)(entry_eric + 8) = fre.raw[1];
  *(uint64_t*)entry_nic = fce[0].raw[0];
  *(uint64_t*)(entry_nic + 8) = fce[0].raw[1];
  __asm__ __volatile__("wbinvd");
  __asm__ __volatile__("wbinvd");
  __asm__ __volatile__("wbinvd");
  __asm__ __volatile__("wbinvd");
  dmar_dump(rtaddr);
  // dump et re vmcall
  return 0;
}

void hook_main(void) {
  hook_boot[EXIT_REASON_EPT_VIOLATION] = &iommu_boot_ept;
  hook_boot[EXIT_REASON_MONITOR_TRAP_FLAG] = &iommu_boot_mtf;
  // hook_boot[EXIT_REASON_VMCALL] = &iommu_boot_vmcall2;
  // fce = efi_allocate_pages(1);
  // Locate eric
  // EFI_GUID guid_eric = EFI_PROTOCOL_ERIC_GUID;
  // EFI_STATUS status = LibLocateProtocol(&guid_eric, (void **)&eric);
  // if (EFI_ERROR(status)) {
  //   ERROR("FAILED LOL LocateProtocol\n");
  // }
  // INFO("ENV INIT : ERIC BAR0 %X\n", eric->bar0);
  // INFO("ENV INIT : ERIC EXP %X\n", eric->rom);
  iommu_protect();
}
