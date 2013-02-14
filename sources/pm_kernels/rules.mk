sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir		:= $(d)/pm_rm_int10
include		$(dir)/rules.mk
dir		:= $(d)/pm_rm_b8000
include		$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
