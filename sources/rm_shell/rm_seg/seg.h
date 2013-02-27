__asm__(".code16gcc\n");
__asm__("mov $0xffff, %ax	;\n"
	"mov %ax, %ds		;\n"
	"mov %ax, %es		;\n"
	"mov %ax, %fs		;\n"
	"mov %ax, %gs		;\n"
	"mov %ax, %ss		;\n"
	"mov $0x00, %ax	;\n"
	"mov %ax, %ds		;\n"
	"mov %ax, %es		;\n"
	"mov %ax, %fs		;\n"
	"mov %ax, %gs		;\n"
	"mov %ax, %ss		;\n"
	"mov $0x2000, %sp;\n");
