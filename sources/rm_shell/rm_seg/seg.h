__asm__(".code16gcc\n");
__asm__("mov $0x00, %ax	;\n"
	"mov %ax, %ds		;\n"
	"mov %ax, %ss		;\n"
	"mov $0x1000, %sp;\n");
