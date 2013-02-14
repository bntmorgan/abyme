sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

#dir		:= $(d)/rm_int10
#include		$(dir)/rules.mk
#dir		:= $(d)/rm_b8000
#include		$(dir)/rules.mk

dir	:= $(d)/rm_kernels
include	$(dir)/rules.mk
dir	:= $(d)/pm_kernels
include	$(dir)/rules.mk
dir	:= $(d)/loader
include	$(dir)/rules.mk
dir	:= $(d)/vmm
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
