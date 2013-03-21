sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir		:= $(d)/pm_rm_pm
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_int10
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_int13
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_int19
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_b8000
include		$(dir)/rules.mk
dir		:= $(d)/pm_bios_rm
include		$(dir)/rules.mk
dir		:= $(d)/pm_bios_rm_int13
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_smm
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_test_libc
include		$(dir)/rules.mk
dir		:= $(d)/pm_b8000
include		$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
