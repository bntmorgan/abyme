sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGET_ELF		  := $(call SRC_2_BIN, $(d)/efi.elf)
TARGETS 				+= $(TARGET)
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/ids.o $(d)/env.o $(d)/env_flash.o \
		$(d)/env_md5.o $(d)/md5.o)

OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -I$(d)/vmm_rec -D_DEBUG_SERVER \
											-U_NO_PRINTK

$(TARGET_ELF)				:  LD_FLAGS_TARGET	:=
$(TARGET_ELF)				:  LD_FLAGS_ALL	:= -nostdlib -T $(LIBVMM_SRC_PATH)/efi.ld \
							-shared -Bsymbolic -L$(EFI_PATH) \
							$(EFI_CRT_OBJS) -znocombreloc -fPIC \
							--no-undefined
$(TARGET_ELF)				:  EFI_LIBS_TARGET := -lvmm -L$(LIBVMM_PATH)

$(TARGET_ELF)			:  LD_OBJECTS	:= $(OBJS_$(d)) 
# Runtime driver
$(TARGET)					:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET_ELF)			:  $(OBJS_$(d)) $(LIBVMM_PATH)/libvmm.a

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
