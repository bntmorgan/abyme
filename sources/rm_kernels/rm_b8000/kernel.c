__asm__(".code16gcc\n");
__asm__("mov $0xb800, %ax	;\n"
	"mov %ax, %ds		;\n"
	"jmpl $0, $main		;\n");

#define __NORETURN __attribute__((noreturn))
 
int __NORETURN main(void) {
  unsigned short *ptr;
  ptr = (unsigned short *) 0x00;
  ptr[0] = 0x0700 | 'a';
  ptr[1] = 0x0700 | 'b';
  ptr[2] = 0x0700 | 'c';
  while (1);
}

