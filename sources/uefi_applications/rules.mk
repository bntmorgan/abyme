sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

dir	:= $(d)/acpi
include	$(dir)/rules.mk
dir	:= $(d)/hello_world
include	$(dir)/rules.mk
dir	:= $(d)/locate
include	$(dir)/rules.mk
dir	:= $(d)/sys_table_2go
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))

