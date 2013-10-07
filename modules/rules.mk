sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

SOURCES_$(d)		:= $(call FIND, $(d)/common)
SOURCES					+= $(SOURCES_$(d))
SOURCES_$(d)		:= $(call FIND, $(d)/uefi_shell_scripts)
SOURCES					+= $(SOURCES_$(d))
SOURCES_$(d)		:= $(call FIND, $(d)/include)
SOURCES					+= $(SOURCES_$(d))

dir	:= $(d)/uefi_drivers
include	$(dir)/rules.mk
dir	:= $(d)/uefi_applications
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
