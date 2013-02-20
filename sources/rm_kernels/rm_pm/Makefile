kernel.bin: kernel.s linker.ld Makefile
	gcc -m32 -c -g -Os -march=i686 -ffreestanding -Wall -Werror -I. -o kernel.o kernel.s -Iinclude
	gcc -m32 -c -g -Os -march=i686 -ffreestanding -Wall -Werror -I. -o kernel_pm.o kernel.c -Iinclude
	ld -melf_i386 -static -Tlinker.ld -nostdlib --nmagic -o kernel.elf kernel.o kernel_pm.o
	objcopy -O binary kernel.elf kernel.bin
	objdump -D -b binary -mi8086 -Maddr16,data16 kernel.bin > kernel.bin.txt
