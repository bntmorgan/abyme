#include "stdio.h"
#include "hook.h"
#include "vmm.h"
#include "ept.h"
#include "vmx.h"
#include "paging.h"
#include "vmcs.h"
#include "efi/efi_eric.h"

#include <efi.h>
#include <efilib.h>

// Mettre la fin de la chanson ERIC !
static char lold[32][4] = {
  {'n', 'e', 'v', 'e'},
  {'r', ' ', 'g', 'o'},
  {'n', 'n', 'a', ' '},
  {'g', 'i', 'v', 'e'},
  {' ', 'y', 'o', 'u'},
  {' ', 'u', 'p', ' '},
  {'n', 'e', 'v', 'e'},
  {'r', ' ', 'g', 'o'},
  {'n', 'n', 'a', ' '},
  {'l', 'e', 't', ' '},
  {'y', 'o', 'u', ' '},
  {'d', 'o', 'w', 'n'},
  {' ', 'n', 'e', 'v'},
  {'e', 'r', ' ', 'g'},
  {'o', 'n', 'n', 'g'},
  {' ', 'r', 'u', 'n'},
  {' ', 'a', 'r', 'o'},
  {'u', 'n', 'd', ' '},
  {'a', 'n', 'd', ' '},
  {'d', 'e', 's', 'e'},
  {'r', 't', ' ', 'y'},
  {'o', 'u', ' ', ' '}
};

static uint8_t sequence[4] = {0x7c, 0x78, 0x74, 0x70};
static uint8_t seq_idx = 0;
static uint8_t unlocked = 0;

protocol_eric *eric;

int ept_boot(struct registers *gr) {
  uint64_t guest_physical_addr = cpu_vmread(GUEST_PHYSICAL_ADDRESS);
  if (((uint64_t)eric->bar0 & ~(uint64_t)0xfff) == (guest_physical_addr &
        ~(uint64_t)0xfff)) {
    uint16_t reg = guest_physical_addr & 0xfff;
    INFO("Access to reg(0x%04x) ", reg);
    uint64_t eq = cpu_vmread(EXIT_QUALIFICATION);
    if (eq & 1) { // READ
      if (unlocked) {
        printk("UNLOCKED READ FOR ERIC\n");
        // XXX YOLO this is everytime r15
        gr->r15 = *(uint32_t*)guest_physical_addr;
        // This is the end of reading : go back to locked state
        if (reg == 0x7c) {
          unlocked = 0;
        }
      } else {
        printk("LOCKED READ FOR ERIC\n");
        // XXX YOLO this is everytime r15
        gr->r15 = (gr->r15 & 0xffffffff00000000) | *((uint32_t*)&lold[reg >> 2]);
      }
    } else if (eq & 2) { // WRITE
      // We override this EPT VIOLATION because it involves ERIC
      if (unlocked) {
        printk("ALLOWED WRITE FOR ERIC\n");
        // XXX YOLO this is everytime rax
        *(uint32_t*)guest_physical_addr = gr->rax & 0xffffffff;
        // XXX decode instruction
        uint64_t a;
        uint64_t *e;
        uint8_t *i;
        uint8_t t;
        // Walk to the instruction
        if (paging_walk(cpu_vmread(GUEST_CR3), gr->rip, &e, &a,&t)) {
          ERROR("impossible to walk to the guest instruction... :(\n");
        }
        INFO("Walk rip(0x%016X => 0x%016X)\n", gr->rip, a);
        i = (uint8_t *)a;
        INFO("i(0x%016X)\n", *(uint64_t*)i);
        if (*(uint16_t*)&i[0] == 0x00c7) { // mov o(%exx) imm32 
          uint32_t v;
          // get immediate value
          v = *(uint32_t*)&i[2];
          printk("Writing 0x%08x to the register 0x%02x\n", v, reg);
          // Write into ERIC
          *(uint32_t*)guest_physical_addr = v;
        }
      } else {
        printk("UNALLOWED WRITE FOR ERIC\n");
      }
      // Sequence check
      if (sequence[0] == reg) {
        seq_idx = 1;
      } else if (sequence[seq_idx] == reg) {
        seq_idx++;
      } else {
        seq_idx = 0;
      }
      if (seq_idx > 3) {
        unlocked = 1;
      }
    } else {
      ERROR("Fetch error :(\n");
    }
    return 1;
  }
  return 0;
}

void hook_main(void) {
  // Get ERIC driver
  EFI_GUID guid_eric = EFI_PROTOCOL_ERIC_GUID;
  EFI_STATUS status = LibLocateProtocol(&guid_eric, (void **)&eric);
  if (EFI_ERROR(status)) {
    ERROR("FAILED LOL LocateProtocol\n");
  }

  INFO("ERIC is @0x%016X\n", (uint64_t)eric->bar0);

  // Protect the memory region we want for every contexts
  uint8_t m;
  for (m = 0; m < VM_NB; m++) {
    ept_perm(((uint64_t)eric->bar0 & (~(uint64_t)0xfff)), 1, 0x0, m);
  }

  // Install some hooks
  hook_boot[EXIT_REASON_EPT_VIOLATION] = &ept_boot;
}
