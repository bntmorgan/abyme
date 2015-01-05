sp 		:= $(sp).x
dirstack_$(sp)	:= $(d)
d		:= $(dir)

SOURCES_$(d)		:= $(call FIND, $(d)/common)
SOURCES					+= $(SOURCES_$(d))
SOURCES_$(d)		:= $(call FIND, $(d)/shell_scripts)
SOURCES					+= $(SOURCES_$(d))
SOURCES_$(d)		:= $(call FIND, $(d)/include)
SOURCES					+= $(SOURCES_$(d))

dir	:= $(d)/drivers
include	$(dir)/rules.mk
dir	:= $(d)/applications
include	$(dir)/rules.mk

d		:= $(dirstack_$(sp))
sp		:= $(basename $(sp))
