CC		= gcc
OBJDUMP		= objdump
CC_FLAGS_ALL	= -Wall -Werror -Werror
CC_FLAGS_16	= -m32 -Os -march=i686 -ffreestanding
CC_FLAGS_32	= -m32 -march=i686
LD_FLAGS_16	= -melf_i386 -static -nostdlib --nmagic
LD_FLAGS_32	= -melf_i386 -static -nostdlib --nmagic
OBJDUMP_FLAGS_16	= -mi8086 -Maddr16,data16

.SUFFIXES: .elf32 .elf16 .bin .bytes

# see objdump -i
# see ld -melf_i386 --verbose
# see ld --verbose

define SRC_2_OBJ
    $(foreach src,$(1),$(patsubst sources/%,build/%,$(src)))
endef

define SRC_2_BIN
    $(foreach src,$(1),$(patsubst sources/%,binary/%,$(src)))
endef

all: kernels

build/%.o: sources/%.s
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_KERNEL) -o $@ -c $<

build/%.o: sources/%.c
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_KERNEL) -o $@ -c $<

binary/%.elf32:
	@echo "  [LD]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(LD) -T$(LD_SCRIPT) $(LD_FLAGS_KERNEL) $(LD_OBJECTS) -o $@

binary/%.elf16:
	@echo "  [LD]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(LD) -T$(LD_SCRIPT) $(LD_FLAGS_KERNEL) $(LD_OBJECTS) -o $@

%.bytes: %.bin
	@echo "  [BYTES] $< -> $@"
	@cat $< | hexdump -v -e '"BYTE(0x" 1/1 "%02X" ");\n"' > $@

%.bin: %.elf16
	@echo "  [BIN]   $< -> $@"
	@objcopy -O binary $< $@

%.bin: %.elf32
	@echo "  [BIN]   $< -> $@"
	@objcopy -O binary $< $@

KERNELS	:=
OBJECTS :=

dir	:= sources
include	$(dir)/rules.mk

kernels: $(patsubst sources/%, binary/%, $(KERNELS))

info:
	@echo Kernels: $(KERNELS)

clean:
	rm -rf $(KERNELS) $(OBJECTS)

usb:
	python tools/config.py config/usb.conf
	sh config/usb.conf.sh

floppy.img:
	$(eval IMG := $(shell echo binary/floppy.img))
	$(eval DEV := $(shell losetup -f))
	dd if=/dev/zero of=$(IMG) bs=1024 count=1440
	losetup $(DEV) $(IMG)
	mkfs $(DEV)
	python tools/config.py config/floppy.img.conf
	sh config/floppy.img.conf.sh
	losetup -d $(DEV)

bochs.floppy.img: floppy.img
	bochs -f config/floppy.img.bochsrc
