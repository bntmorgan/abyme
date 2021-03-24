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

#include <efi.h>
#include <efilib.h>

#include "debug_eth.h"
#include "stdio.h"

void debug_print_reg_stat() {
  INFO("Crc error count 0x%x\n", debug_reg_get(REG_CRCERRS));
  INFO("Reception error count 0x%x\n", debug_reg_get(REG_RXERRC));
  INFO("Missed packet count 0x%x\n", debug_reg_get(REG_MPC));
  INFO("Carrier extention error count 0x%x\n", debug_reg_get(REG_CEXTERR));
  INFO("Receive unsupported count 0x%x\n", debug_reg_get(REG_FCRUC));
  INFO("Good packet received count 0x%x\n", debug_reg_get(REG_GPRC));
}
