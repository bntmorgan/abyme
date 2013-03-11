sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

KERNEL			:= $(call SRC_2_BIN, $(d)/kernel.elf32)
KERNELS			+= $(KERNEL)
OBJS_32_$(d)		:= $(call SRC_2_OBJ, $(d)/loader.o)
OBJECTS			+= $(OBJS_32_$(d))
$(OBJS_32_$(d)):	CC_FLAGS_KERNEL	:= $(CC_FLAGS_32)
$(KERNEL):		LD_FLAGS_KERNEL	:= $(LD_FLAGS_32)
$(KERNEL):		LD_OBJECTS	:= $(OBJS_32_$(d))
$(KERNEL):		LD_SCRIPT	:= $(d)/linker.ld
$(KERNEL):		binary/rm_kernels/rm_test_libc/kernel.bytes
$(KERNEL):		$(OBJS_32_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
