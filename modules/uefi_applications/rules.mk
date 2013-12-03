sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

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
dir	:= $(d)/locate
include	$(dir)/rules.mk
dir	:= $(d)/sys_table_2go
include	$(dir)/rules.mk
dir	:= $(d)/shadowck
include	$(dir)/rules.mk
dir	:= $(d)/test_web
include	$(dir)/rules.mk
dir	:= $(d)/mp
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))

