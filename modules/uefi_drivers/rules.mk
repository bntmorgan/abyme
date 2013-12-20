sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir	:= $(d)/82579LM
include	$(dir)/rules.mk
dir	:= $(d)/uefi_vmm
include	$(dir)/rules.mk
dir	:= $(d)/smm
include	$(dir)/rules.mk
dir	:= $(d)/smm_uefi
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
