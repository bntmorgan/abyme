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
