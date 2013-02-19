sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

#KERNEL			:= $(call SRC_2_BIN, $(d)/kernel.bin)
KERNEL			:= $(call SRC_2_BIN, $(d)/kernel.elf16)
KERNELS			+= $(call SRC_2_BIN, $(KERNEL))
#KERNELS			+= $(call SRC_2_BIN, $(d)/kernel.bin, $(d)/kernel.elf16)
OBJS_16_$(d)		:= $(call SRC_2_OBJ, $(d)/kernel.o)
OBJECTS			+= $(OBJS_16_$(d))
$(OBJS_16_$(d)):	CC_FLAGS_KERNEL	:= $(CC_FLAGS_16)\
									-I$(d)/include
$(KERNEL):		LD_FLAGS_KERNEL	:= $(LD_FLAGS_16)
$(KERNEL):		LD_OBJECTS	:= $(OBJS_16_$(d))
$(KERNEL):		LD_SCRIPT	:= $(d)/linker.ld
$(KERNEL):		$(OBJS_16_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
