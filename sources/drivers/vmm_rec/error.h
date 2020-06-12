#ifndef __ERROR_H__
#define __ERROR_H__

#include "stdio.h"
#include "setup.h"
#include "cpu.h"

#define ERROR_N_REBOOT(...) { \
  PRINTK(0, "[error]",  __VA_ARGS__); \
  union cr4 cr4 = {.raw = cpu_read_cr4()}; \
  if (cr4.vmxe) { \
    REBOOT; \
  } else { \
    printk("[error]failed to reboot, vmm has not started yet\n"); \
    _stdio_stop(); \
  } \
}

#endif//__ERROR_H__
