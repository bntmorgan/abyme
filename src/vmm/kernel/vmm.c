#include "vmm.h"

#include "hardware/cpu.h"
#include "stdio.h"
#include "string.h"
#include "vmm_info.h"

extern uint8_t kernel_start;
extern uint32_t bios_ivt[256];
uint8_t cmos[128];
uint32_t port_previous;
uint32_t value_previous;
uint8_t is_out_previous;
uint8_t in_size_previous;

uint64_t vmm_get_guest_rip(void) {
  uint64_t guest_rip = cpu_vmread(GUEST_RIP);
  uint64_t guest_cr0 = cpu_vmread(GUEST_CR0);
  if ((guest_cr0 & 0x1) == 0x0) {
    uint64_t guest_cs = cpu_vmread(GUEST_CS_SELECTOR);
    /* TODO: if a20, don't wrap! */
    return ((guest_cs << 4) + guest_rip) & 0xfffff;
  } else {
    return guest_rip;
  }
}

void vmm_set_guest_rip(uint64_t guest_rip, uint32_t exit_instruction_length) {
  uint64_t guest_cr0 = cpu_vmread(GUEST_CR0);
  if ((guest_cr0 & 0x1) == 0x0) {
    cpu_vmwrite(GUEST_RIP, (guest_rip + exit_instruction_length) % 0x10000);
  } else {
    cpu_vmwrite(GUEST_RIP, guest_rip + exit_instruction_length);
  }
}

void vmm_read_cmos(void) {
  BREAKPOINT();

/*
 * TODO: pourquoi ca marche pas???
 *  for(uint8_t index = 0; index < 128; index++) {
 *    uint8_t tmp;
 *    __asm__ __volatile__(
 *        "cli             ;"
 *        "out %%al, $0x70 ;"
 *        "in $0x71, %%al  ;"
 *        "sti             ;"
 *      : "=a" (cmos[index]) : "a" (index));
 *  }
 */
  uint8_t tmp;
  __asm__ __volatile__(
      "mov $0x35, %%al ;"
      "out %%al, $0x70 ;"
      "in $0x71, %%al  ;"
    : "=a" (tmp));
  cmos[0x35] = tmp;
  __asm__ __volatile__(
      "mov $0x34, %%al ;"
      "out %%al, $0x70 ;"
      "in $0x71, %%al  ;"
    : "=a" (tmp));
  cmos[0x34] = tmp;
}

void vmm_handle_vm_exit(gpr64_t guest_gpr) {
  guest_gpr.rsp = cpu_vmread(GUEST_RSP);
  uint64_t guest_rip = vmm_get_guest_rip();
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);

  vmm_set_guest_rip(guest_rip, exit_instruction_length);

/*
  INFO("VMEXIT! exit_reason = %d\n", exit_reason);
  INFO("----------\n");
  INFO("rip = 0x%X\n", guest_rip);
  INFO("rsp = 0x%x    rbp = 0x%x\n", guest_gpr.rsp, guest_gpr.rbp);
  INFO("rax = 0x%x    rbx = 0x%x\n", guest_gpr.rax, guest_gpr.rbx);
  INFO("rcx = 0x%x    rdx = 0x%x\n", guest_gpr.rcx, guest_gpr.rdx);
  INFO("rsi = 0x%x    rdi = 0x%x\n", guest_gpr.rsi, guest_gpr.rdi);
  INFO("r8  = 0x%x    r9  = 0x%x\n", guest_gpr.r8,  guest_gpr.r9);
  INFO("r10 = 0x%x    r11 = 0x%x\n", guest_gpr.r10, guest_gpr.r11);
  INFO("r12 = 0x%x    r13 = 0x%x\n", guest_gpr.r12, guest_gpr.r13);
  INFO("r14 = 0x%x    r15 = 0x%x\n", guest_gpr.r14, guest_gpr.r15);
*/

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
    case EXIT_REASON_VMCALL: {
      pmem_mmap_t *pmem_mmap = &vmm_info->pmem_mmap;

      /* TODO: pass an argument to VMCALL */
      if (guest_gpr.rax == 0xe820) {
        INFO("e820 detected!\n");
        if (guest_gpr.rdx != 0x534D4150 /* "SMAP" */ ||
            guest_gpr.rbx >= pmem_mmap->nb_area ||
            guest_gpr.rcx < pmem_mmap->area[guest_gpr.rbx].size) {
          cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) | (1 << 0) /* CF */);
          return;
        }

        uint32_t segment_base = cpu_vmread(GUEST_ES_BASE);
        memcpy((uint8_t*) (segment_base + guest_gpr.rdi), &pmem_mmap->area[guest_gpr.rbx].addr, pmem_mmap->area[guest_gpr.rbx].size);

        guest_gpr.rax = 0x534D4150; /* "SMAP" */
        guest_gpr.rcx = pmem_mmap->area[guest_gpr.rbx].size;
        if (guest_gpr.rbx == pmem_mmap->nb_area - 1) {
          guest_gpr.rbx = 0;
        } else {
          guest_gpr.rbx++;
        }

        cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(1 << 0) /* CF */);
      } else {
        /* Jump to real INT 15h handler */
        cpu_vmwrite(GUEST_CS_SELECTOR, bios_ivt[0x15] >> 16);
        cpu_vmwrite(GUEST_CS_BASE, (bios_ivt[0x15] >> 16) << 4);
        cpu_vmwrite(GUEST_RIP, bios_ivt[0x15] & 0xFFFF);
      }
      break;
    }
    case EXIT_REASON_IO_INSTRUCTION: {
      /* Install new INT 0x15 handler at the end of the BIOS IVT (unused) */
      /* TODO: move it to another place? */
      *((uint8_t*) (255 * 4 + 0)) = 0x0f; /* VMCALL (3 bytes) */
      *((uint8_t*) (255 * 4 + 1)) = 0x01;
      *((uint8_t*) (255 * 4 + 2)) = 0xc1;
      *((uint8_t*) (255 * 4 + 3)) = 0xcf; /* IRET (1 byte) */
      /* Change the INT 0x15 handler address in the BIOS IVT */
      *((uint16_t*) (4 * 0x15 + 2)) = 0;
      *((uint16_t*) (4 * 0x15 + 0)) = 255 * 4;

      /* We don't need to exit on I/O instructions any more */
      cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, cpu_vmread(CPU_BASED_VM_EXEC_CONTROL) & ~USE_IO_BITMAPS);

      /* Replay instruction */
      vmm_set_guest_rip(guest_rip, 0);

      return;

      //INFO("handling IO Ins (rax = %x, rdx = %x, rip = %X)\n", guest_gpr.rax, guest_gpr.rdx, guest_rip);
      if (exit_instruction_length > 4) {
        INFO("unhandled length: %d\n", exit_instruction_length);
        BREAKPOINT();
      } else {
        uint32_t ins = *((uint32_t *) guest_rip);
        uint8_t *ins_byte = (uint8_t *) &ins;
        memset(&ins_byte[exit_instruction_length], 0x90, 4 - exit_instruction_length);

        uint32_t port;
        uint32_t value;
        uint8_t is_out;
        uint8_t in_size;
        if (ins_byte[0] == 0xe6) {
          // out 0x0d, al              ; e60d
          port = ins_byte[1];
          value = guest_gpr.rax & 0xff;
          is_out = 1;
        } else if (ins_byte[0] == 0x66 && ins_byte[1] == 0xed) {
          // in ax, dx                 ; 66ed
          port = guest_gpr.rdx & 0xffff;
          value = guest_gpr.rax & 0xffff;
          is_out = 0;
          in_size = 16;
        } else if (ins_byte[0] == 0xed) {
          // in eax, dx                ; ed
          port = guest_gpr.rdx & 0xffff;
          value = guest_gpr.rax & 0xffffffff;
          is_out = 0;
          in_size = 32;
        } else if (ins_byte[0] == 0xec) {
          // in al, dx                 ; ec
          port = guest_gpr.rdx & 0xffff;
          value = guest_gpr.rax & 0xff;
          is_out = 0;
          in_size = 8;
        } else if (ins_byte[0] == 0xee) {
          // out dx, al                ; ee
          port = guest_gpr.rdx & 0xffff;
          value = guest_gpr.rax & 0xff;
          is_out = 1;
        } else if (ins_byte[0] == 0xe4) {
          // in al, 0x64               ; e464
          port = ins_byte[1];
          value = guest_gpr.rax & 0xff;
          is_out = 0;
          in_size = 8;
        } else {
          INFO("unhandled reason: %d\n", exit_reason);
          INFO("rip = 0x%X\n", guest_rip);
          uint8_t i = 0;
          while (i < exit_instruction_length) {
            printk("%02x", *((uint8_t *)(guest_rip + i)));
            i++;
          }
          printk("\n");
          BREAKPOINT();
        }
        //INFO("IO Ins port=%x value=%x is_out=%x\n", port, value, is_out);
        __asm__ __volatile__(
            "mov %%ecx, 1f(%%rip)  ;"
            "1: nop; nop; nop; nop ;"
          : "=a" (guest_gpr.rax)
          : "a" (guest_gpr.rax), "c" (ins), "d" (guest_gpr.rdx)
          : "rbx");
 
        if (is_out == 0 && port == 0x71 && (is_out_previous == 1 && (value_previous == 0x34 || value_previous == 0xb4))) {
          uint64_t new_value = (uint64_t) &kernel_start;
          uint64_t old_value = (cmos[0x34] + (cmos[0x35] << 8)) * 64 * 1024;
          new_value = old_value - (2 * 1024 * 1024);
          //INFO("before: %X after: %X\n", old_value, new_value);
          // TODO: with shifts!!!
          new_value = new_value / 1024 / 64;
          new_value = new_value & 0xff;
          if (in_size == 8) {
            guest_gpr.rax = (guest_gpr.rax & ~0xff) + new_value;
          } else if (in_size == 16) {
            guest_gpr.rax = (guest_gpr.rax & ~0xffff) + new_value;
          } else if (in_size == 32) {
            guest_gpr.rax = (guest_gpr.rax & ~0xffffffff) + new_value;
          }
          //INFO("read cmos 0x71 0x34 send %x instead of %x\n", new_value, cmos[0x34]);
          //BREAKPOINT();
        } else if (is_out == 0 && port == 0x71 && (is_out_previous == 1 && (value_previous == 0x35 || value_previous == 0xb5))) {
          uint64_t new_value = (uint64_t) &kernel_start;
          uint64_t old_value = (cmos[0x34] + (cmos[0x35] << 8)) * 64 * 1024;
          new_value = old_value - (2 * 1024 * 1024);
          //INFO("before: %X after: %X\n", old_value, new_value);
          // TODO: with shifts!!!
          new_value = new_value / 1024 / 64;
          new_value = (new_value >> 8) & 0xff;
          if (in_size == 8) {
            guest_gpr.rax = (guest_gpr.rax & ~0xff) + new_value;
          } else if (in_size == 16) {
            guest_gpr.rax = (guest_gpr.rax & ~0xffff) + new_value;
          } else if (in_size == 32) {
            guest_gpr.rax = (guest_gpr.rax & ~0xffffffff) + new_value;
          }
          //INFO("read cmos 0x71 0x35 send %x instead of %x\n", new_value, cmos[0x35]);
          //BREAKPOINT();
        }
        port_previous = port;
        value_previous = value;
        is_out_previous = is_out;
        in_size_previous = in_size;
      }
      break;
    }
    default:
      INFO("unhandled reason: %d\n", exit_reason);
      BREAKPOINT();
  }
}
