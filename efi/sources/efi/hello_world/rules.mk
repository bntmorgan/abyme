sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/hello_world.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/hello_world.o)
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_KERNEL	:= 

$(TARGET)				:  LD_FLAGS_KERNEL	:= 
$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
