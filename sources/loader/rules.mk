sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

KERNEL			:= $(call SRC_2_BIN, $(d)/loader.elf32)
KERNELS			+= $(KERNEL)
OBJS_32_$(d)		:= $(call SRC_2_OBJ, $(d)/kernel/kernel.o $(d)/hardware/cpu.o  \
			   $(d)/kernel/loader.o $(d)/kernel/multiboot.o $(d)/common/screen.o \
			   $(d)/common/string.o $(d)/kernel/pmem.o $(d)/kernel/vmem.o     \
			   $(d)/hardware/cpuid.o $(d)/hardware/msr.o $(d)/common/stdio.o $(d)/kernel/elf64.o  \
			   $(d)/common/stdlib.o $(d)/common/keyboard.o)
OBJECTS			+= $(OBJS_32_$(d))
$(OBJS_32_$(d)):	CC_FLAGS_KERNEL	:= -I$(d) -I$(d)/include -m32 -Wall -Wextra -nostdlib \
					   -fno-builtin -nostartfiles -nodefaultlibs -std=c99 \
					   -Wpointer-sign -O0
$(KERNEL):		LD_FLAGS_KERNEL	:= $(LD_FLAGS_32)
$(KERNEL):		LD_OBJECTS	:= $(OBJS_32_$(d))
$(KERNEL):		LD_SCRIPT	:= $(d)/linker.ld
$(KERNEL):		$(OBJS_32_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
