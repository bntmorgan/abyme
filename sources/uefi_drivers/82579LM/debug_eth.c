#include <efi.h>
#include <efilib.h>

#include "debug_eth.h"

inline uint32_t debug_reg_get(uint32_t reg) {
  uint8_t *bar0 = eth_get_bar0();
  return cpu_mem_readd(bar0 + reg);
}

inline void debug_print_reg_stat() {
  Print(L"Crc error count 0x%x\n", debug_reg_get(STAT_CRCERRS));
  Print(L"Reception error count 0x%x\n", debug_reg_get(STAT_RXERRC));
  Print(L"Missed packet count 0x%x\n", debug_reg_get(STAT_MPC));
  Print(L"Carrier extention error count 0x%x\n", debug_reg_get(STAT_CEXTERR));
  Print(L"Receive unsupported count 0x%x\n", debug_reg_get(STAT_FCRUC));
  Print(L"Good packet received count 0x%x\n", debug_reg_get(STAT_GPRC));
  Print(L"Broadcast packet received count 0x%x\n", debug_reg_get(STAT_BPRC));
  Print(L"Multicast packet received count 0x%x\n", debug_reg_get(STAT_MPRC));
  Print(L"Good packet transmitted count 0x%x\n", debug_reg_get(STAT_GPTC));
  Print(L"Good octets received count 0x%x\n", debug_reg_get(STAT_GORCL));
  Print(L"Good octets received count high 0x%x\n", debug_reg_get(STAT_GORCH));
  Print(L"Good octets transmitted count 0x%x\n", debug_reg_get(STAT_GOTCL));
  Print(L"Good octets transmitted count high 0x%x\n", debug_reg_get(STAT_GOTCH));
  Print(L"Receive no buffer count 0x%x\n", debug_reg_get(STAT_RNBC));
  Print(L"Receive undersize count 0x%x\n", debug_reg_get(STAT_RUC));
  Print(L"Receive fragment count 0x%x\n", debug_reg_get(STAT_RFC));
  Print(L"Receive oversize count 0x%x\n", debug_reg_get(STAT_ROC));
  Print(L"Receive jabber count 0x%x\n", debug_reg_get(STAT_RJC));
  Print(L"Management packets receive count 0x%x\n", debug_reg_get(STAT_MNGPRC));
  Print(L"Management packets dropped count 0x%x\n", debug_reg_get(STAT_MNGPDC));
  Print(L"Management packets transmitted count 0x%x\n", debug_reg_get(STAT_MNGPTC));
  Print(L"Tx circuit breaker packets dropped 0x%x\n", debug_reg_get(STAT_TCBPD));
  Print(L"Total octets received 0x%x\n", debug_reg_get(STAT_TORL));
  Print(L"Total octets receiced high 0x%x\n", debug_reg_get(STAT_TORH)); 
  Print(L"Total octets transmitted 0x%x\n", debug_reg_get(STAT_TOTL));
  Print(L"Total octets transmitted high 0x%x\n", debug_reg_get(STAT_TOTH));
  Print(L"Total packets received 0x%x\n", debug_reg_get(STAT_TPR));
  Print(L"Total packets transmitted high 0x%x\n", debug_reg_get(STAT_TPT));
  Print(L"Multicast packets transmitted count 0x%x\n", debug_reg_get(STAT_MPTC));
  Print(L"Broadcast packets received count 0x%x\n", debug_reg_get(STAT_BPTC));
  Print(L"Tcp segmentation context transmitted count 0x%x\n", debug_reg_get(STAT_TSCTC));
  Print(L"Interrupt assertion count 0x%x\n", debug_reg_get(STAT_IAC));
}

void dump(void *fields, uint32_t fds, uint32_t fdss, uint64_t offset, uint32_t step) {
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % 4 == 0) {
      Print(L"%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      Print(L"%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    Print(L" ");
    if (i % 4 == 3) {
      Print(L"\n");
    }
  }
  //if (i % 4 != 3) {
    Print(L"\n");
  //}
}
