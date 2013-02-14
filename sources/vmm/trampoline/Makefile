trampoline.bin: trampoline.s linker.ld Makefile
	gcc -c -O0 -Wall -Werror -I. -o trampoline.o trampoline.s
	ld -static -Tlinker.ld -nostdlib --nmagic -o trampoline.elf trampoline.o
	objcopy -O binary trampoline.elf trampoline.bin
	objdump -D -b binary -mi8086 -Maddr16,data16 trampoline.bin > trampoline.bin.txt
