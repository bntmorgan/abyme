sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/cpu.o $(d)/cpuid.o $(d)/msr.o $(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/mtrr.o $(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/paging.o $(d)/io_bitmap.o)
#OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)

$(TARGET)				:  LD_FLAGS_TARGET	:= 
$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET)				:  $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug
include	$(dir)/rules.mk

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
