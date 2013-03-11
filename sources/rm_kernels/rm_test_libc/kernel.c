#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "seg.h"

__asm__("jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

int __NORETURN main(void) {
  printk("HAAAAHUIEHFUIEZHUIHFUEIZHZFEUIHFUEIHFUIEHFUZEIHFUIEZHIUF\n");
  while(1);
}
