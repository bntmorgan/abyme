sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir		:= $(d)/rm_pm
include		$(dir)/rules.mk
dir		:= $(d)/rm_int10
include		$(dir)/rules.mk
dir		:= $(d)/rm_int13
include		$(dir)/rules.mk
dir		:= $(d)/rm_int19
include		$(dir)/rules.mk
dir		:= $(d)/rm_b8000
include		$(dir)/rules.mk
dir		:= $(d)/rm_smm
include		$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
