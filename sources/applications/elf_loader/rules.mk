sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi.elf)
TARGETS 				+= $(TARGET)
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/common/string.o $(d)/common/stdio.o \
										$(d)/common/screen.o $(d)/common/stdlib.o \
										$(d)/common/debug.o $(d)/common/cpu.o $(d)/common/msr.o \
										$(d)/elf64.o $(d)/loader.o $(d)/common/efiw.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -D_QEMU

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
