sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(notdir $(dir)).elf)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET_ELF))
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/main.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)

$(TARGET_ELF)				:  LD_FLAGS_SCRIPT 	:= -T $(d)/linker.ld

$(TARGET_ELF)				:  LD_FLAGS_TARGET_ELF	:=
$(TARGET_ELF)				:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET_ELF)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
