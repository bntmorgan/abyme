CC	= gcc
CFLAGS	= -m32 -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -std=c99 -Wpointer-sign -O0 -I. -Iinclude
LD	= ld
 
OBJFILES = build/kernel/multiboot.o build/hardware/cpu.o build/kernel/loader.o build/kernel/kernel.o build/common/screen.o build/common/string.o build/kernel/pmem.o build/kernel/vmem.o build/hardware/cpuid.o build/common/stdio.o build/kernel/elf64.o build/common/stdlib.o
 
all: loader.bin

build/%.o: %.s
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<

build/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -S -o $(@).asm -c $<
 
loader.bin: $(OBJFILES)
	$(LD) -melf_i386 -T linker.ld -o $@ $^
 
clean:
	$(RM) $(OBJFILES) loader.bin
