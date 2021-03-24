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

sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir	:= $(d)/elf_loader
include	$(dir)/rules.mk
dir	:= $(d)/netboot
include	$(dir)/rules.mk
dir	:= $(d)/challenge_cpuid
include	$(dir)/rules.mk
dir	:= $(d)/eth_recv
include	$(dir)/rules.mk
dir	:= $(d)/eth_send
include	$(dir)/rules.mk
dir	:= $(d)/acpi
include	$(dir)/rules.mk
dir	:= $(d)/hello_world
include	$(dir)/rules.mk
dir	:= $(d)/cpuid
include	$(dir)/rules.mk
# dir	:= $(d)/locate
# include	$(dir)/rules.mk
dir	:= $(d)/sys_table_2go
include	$(dir)/rules.mk
dir	:= $(d)/shadowck
include	$(dir)/rules.mk
dir	:= $(d)/mp
include	$(dir)/rules.mk
dir := $(d)/ping
include $(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))

