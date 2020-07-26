sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi.elf)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/common/msr.o $(d)/smp.o $(d)/gdt.o $(d)/trampoline.o $(d)/common/string.o $(d)/common/shell.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/cpu.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)

$(TARGET_ELF)				:  LD_FLAGS_TARGET	:= 
$(TARGET_ELF)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET_ELF)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
