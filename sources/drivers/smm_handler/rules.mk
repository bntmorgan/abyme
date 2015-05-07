sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/efi.efi)
TARGETS 				+= $(call SRC_2_BIN, $(TARGET))
OBJS_$(d)				:= $(call SRC_2_OBJ, \
										$(d)/efi.o)
OBJECTS 				+= $(OBJS_$(d))

# Building IA32 binary
$(OBJS_$(d))		:  ARCH						 := ia32
$(OBJS_$(d))		:  EFI_PATH 			 := ./local/lib
$(OBJS_$(d))		:  EFI_INCLUDE		 := ./local/include/efi
$(OBJS_$(d))    :  EFI_INCLUDES 	 := -I$(EFI_INCLUDE) \
		-I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I$(INCLUDE_DIR)
$(OBJS_$(d))		:  CC_FLAGS_TARGET := -I$(d) -m32
$(OBJS_$(d))    :  CC_FLAGS_ALL		 := -Wall -Werror -Werror -O2 \
		-fno-stack-protector -fno-strict-aliasing -fshort-wchar $(EFI_INCLUDES) \
		-fno-builtin -fPIC -O0 -D_DEBUG_SERVER

# Linking IA32 binary
$(TARGET)				:  ARCH						 := ia32
$(TARGET)				:  EFI_PATH 			 := ./local/lib
$(TARGET)				:  LIB_GCC	 			 := ./local/lib/libgcc.a
$(TARGET)				:  EFI_LDS	 			 := efi_ia32.ld
$(TARGET)				:  EFI_CRT_OBJS 	 := \
		./local/src/gnu-efi-3.0/gnuefi/crt0-efi-$(ARCH).o
$(TARGET)				:  EFI_LIBS 			 := -lefi -lgnuefi $(LIB_GCC)
$(TARGET)				:  LD_FLAGS_ALL		:= -nostdlib -T $(EFI_LDS) -shared -Bsymbolic\
		-L$(EFI_PATH) $(EFI_CRT_OBJS) -znocombreloc -fPIC --no-undefined
$(TARGET)				:  LD_FLAGS_TARGET := -melf_i386

$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
# Runtime driver
$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-rtdrv-$(ARCH)
# Bootservice driver
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-bsdrv-$(ARCH)
# Application
#$(TARGET)				:  OBJCPY_FLAGS_TARGET	:= --target=efi-app-$(ARCH)
$(TARGET)				:  $(OBJS_$(d))

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
