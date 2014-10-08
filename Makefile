CC						:= gcc
OBJDUMP				:= objdump
PYTHON				:= python3

INCLUDE_DIR			:= sources/include

ARCH := $(shell uname -m | sed s,i[3456789]86,ia32,)

PREFIX					:= /usr
EFI_INCLUDE			:= $(PREFIX)/include/efi
# Installation gnu-efi 3.0 locale
# EFI_INCLUDE			:= /usr/local/include/efi

EFI_INCLUDES 		:= -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) \
		-I$(EFI_INCLUDE)/protocol -I$(INCLUDE_DIR)

EFI_PATH 				:= $(PREFIX)/lib
# Installation gnu-efi 3.0 locale
# EFI_PATH 				:= /usr/local/lib

LIB_GCC	 				:= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS 				:= -lefi -lgnuefi $(LIB_GCC)

EFI_CRT_OBJS 		:= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS 				:= efi.ld

CC_FLAGS_ALL		:= -Wall -Werror -Werror -O2 -fno-stack-protector \
		-fno-strict-aliasing -fshort-wchar $(EFI_INCLUDES) -fno-builtin -fPIC -O0

ifeq ($(ARCH),x86_64)
	CC_FLAGS_ALL	+= -DEFI_FUNCTION_WRAPPER
endif

LD_FLAGS_ALL		:= -nostdlib -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) \
		$(EFI_CRT_OBJS) -znocombreloc -fPIC --no-undefined

define SRC_2_OBJ
  $(foreach src,$(1),$(patsubst modules/%,build/%,$(src)))
endef

define SRC_2_BIN
  $(foreach src,$(1),$(patsubst modules/%,binary/%,$(src)))
endef

define MOD_2_SRC
  $(foreach src,$(1),$(patsubst modules/%,sources/%,$(src)))
endef

define SVG_2_PDF
  $(foreach src,$(1),$(patsubst modules/%.svg,sources/%.pdf,$(src)))
endef

define FIND
	$(shell find $(1) -type f | grep -v '/\.' | grep -v 'mk$$' | grep -v \
			'\.w$$' | grep -v '\.svg$$')
endef

define FIND_FIGURES
  $(shell find $(1) -type f | grep -i '\.svg$$')
endef

all: targets
# make -C test

# Overriden in rules.mk
TARGETS :=
OBJECTS :=
WEBS		:=
FIGS 		:=
SOURCES :=

dir	:= modules
target_dir := sources
include	$(dir)/rules.mk

sources/%temoin:
	@for w in $^; do \
		filename=`basename $$w`; \
		if [ "$${filename%.*}" = "root" ]; then \
			echo '  [W@]    '$$w; \
			$(PYTHON) literale/prepare.py -b `dirname $$w` `basename $$w` | \
					$(PYTHON) literale/tangle.py -d $(dir $@); \
			$(PYTHON) literale/prepare.py -b `dirname $$w` `basename $$w` | \
					$(PYTHON) literale/weave.py latex > $(dir $@)/doc.tex; \
			touch $@; \
			#echo '  [TX]    '$(dir $@)doc.tex; \
			#cd $(dir $@) && pdflatex doc.tex > /dev/null && pdflatex doc.tex > \
			#		/dev/null;\
		fi \
	done

# Copy rule
sources: $(call MOD_2_SRC, $(SOURCES))

build/%.o: sources/%.s
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_TARGET) -o $@ -c $<

build/%.o: sources/%.c
	@echo "  [CC]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_TARGET) -o $@ -c $<

binary/%.efi: binary/%.elf
	@objcopy -j .padding_begin -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .padding_end \
		$(OBJCPY_FLAGS_TARGET) \
	  $< $@
		@cp $< $<.toto

	@strip $@

binary/%.elf:
	@echo "  [LD]    $@"
	@mkdir -p $(dir $@)
	@$(LD) $(LD_FLAGS_ALL) $(LD_FLAGS_TARGET) $(LD_OBJECTS) -o $@ $(EFI_LIBS)

tests/%:
	@echo "  [LD]    $< -> $@"
	@mkdir -p $(dir $@)
	@$(LD) $(LD_FLAGS_ALL) $(LD_FLAGS_TARGET) $(LD_OBJECTS) -o $@ $(EFI_LIBS)

sources/%.pdf: modules/%.svg
	@mkdir -p $(dir $@)
	@echo "  [IS]    $^ -> $@"
	@inkscape $< --export-pdf="$@"

sources/%: modules/%
	@mkdir -p $(dir $@)
	@cp -r --force $^ $@
	@echo "  [CP]    $^ -> $@"

targets: sources $(WEBS) $(patsubst sources/%, binary/%, $(TARGETS))

clean:
	@rm -fr sources
	@rm -f $(TARGETS) $(OBJECTS)

info:
	@echo Targets [$(TARGETS)]
	@echo Objects [$(OBJECTS)]
	@echo Webs [$(WEBS)]
	@echo Sources [$(SOURCES)]
	@echo Figures [$(FIGS)]

usb: all
	umount /dev/sdb1 || true
	sudo mount /dev/sdb1 /mnt
	sudo cp -r binary/* /mnt/EFI
	sudo cp sources/uefi_shell_scripts/*.nsh /mnt
	sudo umount /mnt

qemu: launch

pre-launch:
	rm -rf qemu/roms/*
	cp qemu/packages/OVMF-X64-r11337-alpha.zip qemu/roms
	cd qemu/roms ; unzip OVMF-X64-r11337-alpha.zip
	rm qemu/roms/OVMF-X64-r11337-alpha.zip
	mv qemu/roms/CirrusLogic5446.rom qemu/roms/vgabios-cirrus.bin
	mv qemu/roms/OVMF.fd qemu/roms/bios.bin
#	cp qemu/edk2/Build/OvmfX64/DEBUG_GCC46/FV/OVMF.fd qemu/roms/bios.bin
	cp /usr/share/qemu/kvmvapic.bin qemu/roms
	cp /usr/share/qemu/pxe-rtl8139.rom qemu/roms
	cp -r binary/ qemu/hda-contents

launch: pre-launch
	qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666 -cpu qemu64,model=6,+vmx -monitor stdio -m 8G \
			#-enable-kvm

#pre-launch:
#	rm -rf qemu/roms/*
#	cp qemu/packages/OVMF-X64-r11337-alpha.zip qemu/roms
#	cd qemu/roms ; unzip OVMF-X64-r11337-alpha.zip
#	rm qemu/roms/OVMF-X64-r11337-alpha.zip
#	mv qemu/roms/CirrusLogic5446.rom qemu/roms/vgabios-cirrus.bin
#	mv qemu/roms/OVMF.fd qemu/roms/bios.bin
#	cp /usr/share/qemu/kvmvapic.bin qemu/roms
#	cp /usr/share/qemu/pxe-rtl8139.rom qemu/roms
#	cp -r binary/ qemu/hda-contents

#launch: pre-launch
#	qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666 -cpu qemu64,model=6,+vmx,+pdpe1gb -monitor stdio -S
# qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666 -cpu qemu64,+sse2 -D /tmp/gg
# qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666 -cpu qemu64,model=3
# qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666
# qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents -gdb \
			tcp:localhost:6666 -s -S
# qemu-system-x86_64 -cpu host -L qemu/roms -hda fat:qemu/hda-contents \
			-monitor telnet:127.0.0.1:5555,server,nowait -gdb tcp::6666 -s -S
# –monitor telnet:127.0.0.1:5555,server,nowait
#qemu-system-x86_64 -L qemu/roms -hda fat:qemu/hda-contents
#qemu-system-x86_64 -L qemu/roms -hda fat:qemu/hda-contents -no-kvm
