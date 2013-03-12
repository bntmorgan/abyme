#include "types.h"
#include "stdio.h"
#include "screen.h"
#include "stdlib.h"
#include "seg.h"

__asm__("jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

int __NORETURN main(void) {
  screen_clear();
  printk("Time to own the bios...\n");
  printk("Bios owned\n:))\n");
  printk("bonjour\n");
  printk("Time to own the bios...\n");
  printk("Bios owned\n:))\n");
  printk("bonjour\n");
  printk("Time to own the bios...\n");
  printk("Bios owned\n:))\n");
  printk("bonjour\n");
  printk("Time to own the bios...\n");
  printk("Bios owned\n:))\n");
  printk("bonjour\n");

  while(1);
}
