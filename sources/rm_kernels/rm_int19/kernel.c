__asm__(".code16gcc\n");
__asm__("mov $0x00, %ax	;\n"
	"mov %ax, %ds		;\n"
	"mov %ax, %ss		;\n"
	"mov $0x1000, %sp;\n"
	"jmpl $0x0, $main	;\n");

#define __NOINLINE __attribute__((noinline))
#define __REGPARM  __attribute__((regparm(3)))
#define __NORETURN __attribute__((noreturn))

/* BIOS interrupts must be done with inline assembly */
void __NOINLINE __REGPARM print(const char *s) {
  while (*s) {
    __asm__ __volatile__ ("int $0x10" : : "a"(0x0E00 | *s), "b"(7));
    s++;
  }
}

int __NORETURN main(void) {
  print("int 19 not failed yet\r\n");
  __asm__ __volatile__ ("int $0x19");
  print("int 19 failed\r\n");
  while (1);
}

