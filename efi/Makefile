CC						:= gcc
OBJDUMP				:= objdump

ARCH := $(shell uname -m | sed s,i[3456789]86,ia32,)

EFI_INCLUDE			:= /usr/include/efi
INCLUDE_DIR			:= sources/include
EFI_INCLUDES 		:= -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I$(INCLUDE_DIR)
EFI_PATH 				:= /usr/lib

LIB_GCC	 				:= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS 				:= -lefi -lgnuefi $(LIB_GCC)

EFI_CRT_OBJS 		:= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS 				:= $(EFI_PATH)/elf_$(ARCH)_efi.lds

CC_FLAGS_ALL		:= -Wall -Werror -Werror -O2 -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar $(EFI_INCLUDES)

ifeq ($(ARCH),x86_64)
	CC_FLAGS_ALL	+= -DEFI_FUNCTION_WRAPPER
endif

LD_FLAGS_ALL		:= -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) $(EFI_CRT_OBJS)

define SRC_2_OBJ
    $(foreach src,$(1),$(patsubst sources/%,build/%,$(src)))
endef

define SRC_2_BIN
    $(foreach src,$(1),$(patsubst sources/%,binary/%,$(src)))
endef

all: targets

# Overriden in rules.mk
TARGETS :=
OBJECTS :=

dir	:= sources
include	$(dir)/rules.mk

build/%.o: sources/%.s
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_KERNEL) -o $@ -c $<

build/%.o: sources/%.c
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_KERNEL) -o $@ -c $<

binary/%.efi:
	@echo "  [LD]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(LD) $(LD_FLAGS_ALL) $(LD_FLAGS_KERNEL) $(LD_OBJECTS) -o $@ $(EFI_LIBS)
	@objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc \
		$(OBJCPY_FLAGS_KERNEL) $@
	@strip $@

targets: $(patsubst sources/%, binary/%, $(TARGETS))

clean:
	rm -f $(TARGETS) $(OBJECTS)

info:
	@echo Targets [$(TARGETS)]
	@echo Objects [$(OBJECTS)]

usb: all
	sudo mount /dev/sdb1 /mnt && sudo cp -r binary/efi/* /mnt/EFI/ && sudo umount /mnt
