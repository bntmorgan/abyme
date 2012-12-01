trampoline.bin: trampoline.c linker.ld Makefile
	gcc -std=c99 -c -g -fpic -O0 -march=i686 -ffreestanding -Wall -Werror -I. -o trampoline.o trampoline.c
	ld -static -Tlinker.ld -nostdlib --nmagic -o trampoline.elf trampoline.o
	objcopy -O binary trampoline.elf trampoline.bin
