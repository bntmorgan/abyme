CC	= gcc
CFLAGS	= -m32 -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -std=c99 -Wpointer-sign -O0 -I.
LD	= ld
 
OBJFILES = build/kernel/multiboot.o build/arch/cpu.o build/kernel/loader.o build/kernel/kernel.o build/common/screen.o build/common/string.o build/kernel/pmem.o build/kernel/vmem.o build/kernel/mod.o build/arch/cpuid.o
 
all: loader.bin

build/%.o: %.s
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<

build/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<
 
loader.bin: $(OBJFILES)
	$(LD) -melf_i386 -T linker.ld -o $@ $^
 
clean:
	$(RM) $(OBJFILES) loader.bin
