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

TARGET					:= $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(notdir $(dir)).efi.elf)
TARGETS 				+= $(TARGET)
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/ids.o)

OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -I$(d)/vmm_rec -U_DEBUG_SERVER \
											-U_NO_PRINTK

$(TARGET_ELF)				:  LD_FLAGS_SCRIPT 	:= -T $(LIBVMM_SRC_PATH)/efi.ld
$(TARGET_ELF)				:  EFI_LIBS_TARGET := -lvmm -L$(LIBVMM_PATH)

$(TARGET_ELF)			:  LD_OBJECTS	:= $(OBJS_$(d)) 
# Runtime driver
$(TARGET)					:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET_ELF)			:  $(OBJS_$(d)) $(LIBVMM_PATH)/libvmm.a

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
