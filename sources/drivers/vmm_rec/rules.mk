# Copyright (C) 2021  Benoît Morgan
#
# This file is part of abyme
#
# abyme is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# abyme is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with abyme.  If not, see <http://www.gnu.org/licenses/>.

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET_A			  := $(call SRC_2_BIN, $(d)/libvmm.a)
TARGETS 				+= $(TARGET_A)
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o $(d)/vmm.o $(d)/common/microudp.o $(d)/common/arp.o $(d)/common/icmp.o $(d)/common/efiw.o $(d)/common/shell.o $(d)/common/stdio.o $(d)/common/stdlib.o $(d)/common/debug.o $(d)/common/cpu.o $(d)/common/cpuid.o $(d)/common/msr.o $(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/common/mtrr.o $(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/common/paging.o $(d)/common/pat.o $(d)/io_bitmap.o $(d)/pci.o $(d)/vmx.o $(d)/nested_vmx.o $(d)/idt.o $(d)/isr.o $(d)/apic.o $(d)/dmar.o $(d)/hook.o $(d)/reboot.o $(d)/common/serial.o)

OBJECTS 				+= $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug_server
include	$(dir)/rules.mk

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -U_DEBUG_SERVER -U_NO_PRINTK \
	-D_VMCS_SHADOWING -U_PARTIAL_VMX -D_NESTED_EPT -U_NO_GUEST_EPT -U_ESXI \
	-D_DEBUG -D_NO_MSR_PLATFORM_INFO -D_CPU_FREQ_MHZ=2100 -U_NO_TSC_OFFSETTING

$(TARGET_A)  			:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET_A)			:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
