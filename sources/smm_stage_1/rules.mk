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

# ia32e mode

T := $(notdir $(dir))_64

TARGET					:= $(call SRC_2_BIN, $(d)/$(T).bin)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(T).elf)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/hook_64.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -fPIC -fPIE

$(TARGET_ELF)				:  LD_FLAGS_SCRIPT 	:= -T $(d)/linker_64.ld

$(TARGET_ELF)				:  LD_FLAGS_TARGET_ELF	:=
$(TARGET_ELF)				:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET_ELF)				:  $(OBJS_$(d))

# protected mode

T := $(notdir $(dir))_32

TARGET					:= $(call SRC_2_BIN, $(d)/$(T).bin)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/$(T).elf)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/hook_32.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -fPIC -fPIE # -m32 -march=i686

$(TARGET_ELF)				:  LD_FLAGS_SCRIPT 	:= -T $(d)/linker_32.ld # -melf_i386

$(TARGET_ELF)				:  LD_FLAGS_TARGET_ELF	:= # -m32
$(TARGET_ELF)				:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET_ELF)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
