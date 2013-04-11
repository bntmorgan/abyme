sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/hardware/cpu.o $(d)/hardware/cpuid.o $(d)/hardware/msr.o $(d)/vmm_vm_exit_handler.o $(d)/vmm_setup.o $(d)/vmm_vmcs.o $(d)/vmem.o $(d)/mtrr_new.o $(d)/mtrr.o $(d)/common/string.o)
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

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))