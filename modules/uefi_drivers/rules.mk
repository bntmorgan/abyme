sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir	:= $(d)/cmos
include	$(dir)/rules.mk
dir	:= $(d)/flash
include	$(dir)/rules.mk
dir	:= $(d)/82579LM
include	$(dir)/rules.mk
dir	:= $(d)/uefi_vmm
include	$(dir)/rules.mk
dir	:= $(d)/uefi_vmm_supervisor
include	$(dir)/rules.mk
dir	:= $(d)/smm
include	$(dir)/rules.mk
dir	:= $(d)/uefi_smm
include	$(dir)/rules.mk
# dir	:= $(d)/uefi_smm_handler
# include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
