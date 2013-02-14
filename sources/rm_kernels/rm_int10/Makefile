pepin.bin: pepin.c linker.ld Makefile
	gcc -m32 -c -g -Os -march=i686 -ffreestanding -Wall -Werror -I. -o pepin.o pepin.c
	ld -melf_i386 -static -Tlinker.ld -nostdlib --nmagic -o pepin.elf pepin.o
	objcopy -O binary pepin.elf pepin.bin
	objdump -D -b binary -mi8086 -Maddr16,data16 pepin.bin > pepin.bin.txt
