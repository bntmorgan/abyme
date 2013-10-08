sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/cpu.o $(d)/cpuid.o $(d)/msr.o $(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/mtrr.o $(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/paging.o $(d)/io_bitmap.o $(d)/pci.o, $(d)/test.o $(d)/smp.o)

OBJECTS 				+= $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug_server
include	$(dir)/rules.mk

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)

# Old Style C files
SOURCES_$(d)		:= $(call FIND, $(d))
SOURCES					+= $(SOURCES_$(d))

# New awesome rocking WEB
WEB							:= $(call MOD_2_SRC, $(d)/temoin)
WEBS						+= $(WEB)
WEBS_$(d)				:= $(d)/root.w
$(WEB)					:	 $(WEBS_$(d))

$(TARGET)				:  LD_FLAGS_TARGET	:= 
$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET)				:  $(SOURCES_$(d)) $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
