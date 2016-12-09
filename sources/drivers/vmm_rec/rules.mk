sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET_A			  := $(call SRC_2_BIN, $(d)/libvmm.a)
TARGETS 				+= $(TARGET_A)
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/microudp.o $(d)/common/arp.o $(d)/common/icmp.o $(d)/common/efiw.o $(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/common/cpu.o $(d)/common/cpuid.o $(d)/common/msr.o $(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/common/mtrr.o $(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/common/paging.o $(d)/common/pat.o $(d)/io_bitmap.o $(d)/pci.o $(d)/vmx.o $(d)/nested_vmx.o $(d)/idt.o $(d)/isr.o $(d)/apic.o $(d)/dmar.o $(d)/hook.o $(d)/reboot.o)

OBJECTS 				+= $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug_server
include	$(dir)/rules.mk

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -D_DEBUG_SERVER -U_NO_PRINTK \
	-D_VMCS_SHADOWING -U_PARTIAL_VMX -D_NESTED_EPT -U_NO_GUEST_EPT -U_ESXI \
	-D_DEBUG -D_NO_MSR_PLATFORM_INFO -D_CPU_FREQ_MHZ=2100

$(TARGET_A)  			:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET_A)			:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
