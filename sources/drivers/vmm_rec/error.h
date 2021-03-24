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
