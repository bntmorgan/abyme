#include "types.h"
#include "seg.h"
#include "common/own_bios.h"

__asm__("jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

typedef struct _dap {
  uint8_t size; /* Size of DAP = 16, 0x10 bytes */
  uint8_t unused; /* Should be zero */
  uint16_t nb_sectors_to_read; /* some BIOSes limit it to 127 */
  uint16_t buffer_low; /* segment:offset pointer to the memory buffer to which sectors 
                      will be transferred (note that x86 is little-endian: if declaring 
                      the segment and offset separately, the offset must be 
                      declared before the segment) */
  uint16_t buffer_high;
  uint64_t offset; /* The segment offset where to start the reading */
} __attribute__((packed)) dap;

/* BIOS interrupts must be done with inline assembly */
void __NOINLINE __REGPARM print(const char *s) {
  while (*s) {
    __asm__ __volatile__ ("int $0x10" : : "a"(0x0E00 | *s), "b"(7));
    s++;
  }
}

void itoa(int8_t *dst, uint8_t base, int32_t value) {
  uint32_t uvalue = (uint32_t) value;
  int8_t *ptr = dst;
  /*
   * Set the sign for decimal values.
   */
  if (base == 10 && value < 0) {
    *dst = '-';
    dst = dst + 1;
    ptr = ptr + 1;
    uvalue = -value;
  }
  /*
   * Transform into string in reverse order.
   */
  do {
    *ptr = uvalue % base;
    if (*ptr < 10) {
      *ptr = *ptr + '0';
    } else {
      *ptr = *ptr + 'a' - 10;
    }
    ptr = ptr + 1;
    uvalue = uvalue / base;
  } while (uvalue > 0);
  *ptr = 0;
  ptr = ptr - 1;
  /*
   * Correct order of the string.
   */
  while (dst < ptr) {
    uint8_t tmp = *ptr;
    *ptr = *dst;
    *dst = tmp;
    dst = dst + 1;
    ptr = ptr - 1;
  }
}

int __NOINLINE __REGPARM read_first_sector(uint8_t *sector) {
  // Dap structure
  dap d;
  d.size = 0x10;
  d.unused = 0x0;
  d.nb_sectors_to_read = 0x1;
  d.buffer_high = 0x0; // Segment 0x0
  d.buffer_low = (uint16_t) (uint32_t)sector; // Offset address of sector in the stack
  d.offset = 0;
  uint8_t c = 0;
  uint16_t ret = 0; 
  __asm__ __volatile__ ("mov %0, %%esi" : : "a"((uint16_t*)&d));
  __asm__ __volatile__ ("xchg %bx, %bx");
  __asm__ __volatile__ ("int $0x13" : "=cc" (c), "=a" (ret) : "a" (0x4200), "d" (0x80));
    
  print((char *)&d);

  if (c) {
    return ret;
  } else {
    return 0;
  }
}

//#define DEBUG

int __NORETURN main(void) {
  print("Time to own the bios...\r\n");
  own_bios(0xf831f);
  print("end\r\n:))");
  // Sector read
  print("bonjour\n");
  uint8_t sector[512]; // The sector data
  if (read_first_sector(sector)) {
    print("FAILED\r\n");
  } else {
    print("SUCCESS\r\n");
    uint32_t i;
    char buf[11];
    for (i = 0; i < 0x10; i++) {
    // Index
#ifdef DEBUG
      print("sector[");
      itoa(buf, 16, i);
      buf[8] = ' ';
      buf[9] = '\0';
      print(buf);
      print("] = ");
#endif
      // 4 Octets du secteur
      itoa(buf, 16, ((uint32_t *)sector)[i]);
      buf[8] = '\r';
      buf[9] = '\n';
      buf[10] = '\0';
      print(buf);
#ifdef DEBUG
      print("\r\n");
#endif
#ifndef DEBUG
      print(", ");
#endif
    }
  }
  print("end\r\n:))");
  while (1);
}

void hook_bios() {
  print("hook bios\r\n");
  // Never return
  while(1);
}
