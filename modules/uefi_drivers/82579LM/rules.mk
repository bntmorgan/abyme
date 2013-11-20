sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/82579LM.o $(d)/common/cpu.o $(d)/pci.o $(d)/debug_eth.o $(d)/api.o $(d)/common/string.o $(d)/common/stdio.o $(d)/common/pat.o $(d)/common/mtrr.o $(d)/common/msr.o $(d)/common/paging.o $(d)/common/screen.o $(d)/common/stdlib.o $(d)/common/cpuid.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)
SOURCES_$(d)		:= $(call FIND, $(d))
SOURCES					+= $(SOURCES_$(d))

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
