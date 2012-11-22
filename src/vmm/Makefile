CC	= gcc
CFLAGS	= -m64 -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -std=c99 -Wpointer-sign -O0 -I.
LD	= ld
 
OBJFILES = build/kernel/start.o build/kernel/kernel.o build/common/screen.o build/common/string.o build/arch/cpu.o build/kernel/vmem.o build/kernel/pmem.o build/arch/msr.o build/kernel/vmm.o
 
all: vmm.bin
 
build/%.o: %.s
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<

build/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<

arch/msr.t: arch/msr.c
	$(CC) -g -S $(CFLAGS) -o $@ -c $<
 
kernel/vmm.t: kernel/vmm.c
	$(CC) -g -S $(CFLAGS) -o $@ -c $<
 
vmm.bin: $(OBJFILES)
	$(LD) -T linker.ld -o $@ $^
 
clean:
	$(RM) $(OBJFILES) vmm.bin
