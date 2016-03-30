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
# dir	:= $(d)/smm_handler
# include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
