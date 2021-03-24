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

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/efi.elf)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
											$(d)/efi.o $(d)/vmm.o $(d)/common/efiw.o \
	$(d)/common/screen.o $(d)/common/stdio.o $(d)/common/stdlib.o \
	$(d)/common/debug.o $(d)/common/cpu.o $(d)/common/cpuid.o $(d)/common/msr.o \
	$(d)/vmexit.o $(d)/setup.o $(d)/vmcs.o $(d)/gdt.o $(d)/common/mtrr.o \
	$(d)/common/string.o $(d)/ept.o $(d)/msr_bitmap.o $(d)/common/paging.o \
	$(d)/common/pat.o $(d)/io_bitmap.o $(d)/pci.o \
	$(d)/vmx.o $(d)/env.o $(d)/env_flash.o $(d)/env_md5.o $(d)/md5.o $(d)/walk.o)

OBJECTS 				+= $(OBJS_$(d))

# Debugger
dir	:= $(d)/debug_server
include	$(dir)/rules.mk

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)/include_challenge -I$(d) \
	-D_DEBUG_SERVER -U_NO_PRINTK -U_LOG_CR3 -D_ENV -DARCH_IS_BIG_ENDIAN=0 -U_DEBUG

$(TARGET_ELF)				:  LD_FLAGS_TARGET	:=
$(TARGET_ELF)				:  LD_FLAGS_ALL	:= -nostdlib -T $(d)/efi.ld \
							-shared -Bsymbolic -L$(EFI_PATH) \
							$(EFI_CRT_OBJS) -znocombreloc -fPIC \
							--no-undefined

$(TARGET_ELF)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET_ELF)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
