sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir	:= $(d)/drivers
include	$(dir)/rules.mk
dir	:= $(d)/applications
include	$(dir)/rules.mk
dir	:= $(d)/smm_stage_1
include	$(dir)/rules.mk
dir	:= $(d)/smm_stage_2
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
