sp              := $(sp).x
dirstack_$(sp)  := $(d)
parent					:= $(d)
d               := $(dir)

OBJS_$(parent)	+= $(call SRC_2_OBJ, \
										$(d)/debug_server.o)

OBJECTS 				+= $(OBJS_$(parent))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
