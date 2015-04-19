sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/efiw.o $(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/common/cpu.o $(d)/common/cpuid.o $(d)/common/msr.o $(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/common/mtrr.o $(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/common/paging.o $(d)/common/pat.o $(d)/io_bitmap.o $(d)/pci.o $(d)/vmx.o $(d)/nested_vmx.o $(d)/idt.o $(d)/isr.o)

OBJECTS 				+= $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug_server
include	$(dir)/rules.mk

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -D_DEBUG_SERVER -U_NO_PRINTK \
	-D_VMCS_SHADOWING -U_PARTIAL_VMX -D_NESTED_EPT

# Old Style C files
SOURCES_$(d)		:= $(call FIND, $(d))
SOURCES					+= $(SOURCES_$(d))

# New awesome rocking WEB
WEB							:= $(call MOD_2_SRC, $(d)/temoin)
WEBS						+= $(WEB)
WEBS_$(d)				:= $(d)/root.w $(d)/smp.w $(d)/multicore.w $(d)/trampoline.w
FIGS_$(d)				:= $(call SVG_2_PDF, $(call FIND_FIGURES, $(d)/figures))
FIGS 						+= $(FIGS_$(d))
$(WEB)					:	 $(FIGS_$(d)) $(WEBS_$(d))

$(TARGET)				:  LD_FLAGS_TARGET	:=
$(TARGET)				:  LD_FLAGS_ALL	:= -nostdlib -T $(d)/efi.ld \
							-shared -Bsymbolic -L$(EFI_PATH) \
							$(EFI_CRT_OBJS) -znocombreloc -fPIC \
							--no-undefined

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
