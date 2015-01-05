sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
WEB							:= $(call MOD_2_SRC, $(d)/temoin)

TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
WEBS						+= $(WEB)

OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/test.o)
WEBS_$(d)				:= $(d)/root.w $(d)/next_file.w
OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d)

$(TARGET)				:  LD_FLAGS_TARGET	:= 
$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET)				:  $(OBJS_$(d))
$(WEB)					:	 $(WEBS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
