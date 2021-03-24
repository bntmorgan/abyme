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

dir	:= $(d)/cmos
include	$(dir)/rules.mk
dir	:= $(d)/flash
include	$(dir)/rules.mk
dir	:= $(d)/pcie_root_port
include	$(dir)/rules.mk
dir	:= $(d)/eric
include	$(dir)/rules.mk
dir	:= $(d)/host_bridge
include	$(dir)/rules.mk
dir	:= $(d)/82579LM
include	$(dir)/rules.mk
# dir	:= $(d)/vmm
# include	$(dir)/rules.mk
# dir	:= $(d)/vmm_supervisor
# include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_iommu
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_none
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_env
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_0
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_1
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_2
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_eric
include	$(dir)/rules.mk
dir	:= $(d)/smm
include	$(dir)/rules.mk
dir	:= $(d)/vmm_rec_cache
include	$(dir)/rules.mk
# dir	:= $(d)/smm_handler
# include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
