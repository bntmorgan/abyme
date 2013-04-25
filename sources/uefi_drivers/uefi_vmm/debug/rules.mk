sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/debug.o)

OBJECTS 				+= $(OBJS_$(d))

test_debug:
	@echo $(d) $(OBJECTS)
	@echo local OBJECTS $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
