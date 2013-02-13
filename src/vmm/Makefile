CC	= gcc
CFLAGS =  -I. -Iinclude -Wall -W -pipe -nostdlib -nostdinc -ffreestanding -fms-extensions -m64 -mno-red-zone -mcmodel=small -fpie -std=c99 -D__X86_64__
LD	= ld
 
OBJFILES = build/kernel/kernel.o build/common/screen.o build/common/string.o build/hardware/cpu.o build/kernel/vmem.o build/kernel/pmem.o build/hardware/msr.o build/kernel/vmm.o build/kernel/vmm_setup.o build/kernel/vmm_vmcs.o build/kernel/smp.o build/common/stdio.o build/common/stdlib.o build/kernel/start.o build/kernel/vmm_vm_exit_handler.o build/kernel/debug.o

 
all: vmm.bin
 
build/%.o: %.s
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<

build/%.o: %.c
	mkdir -p `dirname $@`
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -S -o $(@).asm -c $<

hardware/msr.t: hardware/msr.c
	$(CC) -g -S $(CFLAGS) -o $@ -c $<
 
kernel/vmm.t: kernel/vmm.c
	$(CC) -g -S $(CFLAGS) -o $@ -c $<
 
vmm.bin: $(OBJFILES) trampoline/trampoline.bin pepin/pepin.bin
	cat trampoline/trampoline.bin | hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' > trampoline_bytes.ld
	cat pepin/pepin.bin | hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' > pepin_bytes.ld
	$(LD)  --warn-common --no-check-sections -n -melf_x86_64 -pie -T linker.ld -o $@ $(OBJFILES)

pepin/pepin.bin: pepin/pepin.c
	make -C pepin

trampoline/trampoline.bin: trampoline/trampoline.s
	make -C trampoline

clean:
	$(RM) $(OBJFILES) vmm.bin $(OBJFILES:=.asm)
