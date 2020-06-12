cc						:= gcc
AR						:= ar
OBJDUMP				:= objdump
PYTHON				:= python3
USB						:= /dev/sdb1
# SSD Optimisation
BUILD_DIR			:= /tmp/build
BINARY_DIR		:= /tmp/binary
# Lib VMM
LIBVMM_PATH 		:= $(BINARY_DIR)/drivers/vmm_rec
LIBVMM_SRC_PATH	:= sources/drivers/vmm_rec

INCLUDE_DIR			:= sources/include

ARCH := $(shell uname -m | sed s,i[3456789]86,ia32,)

PREFIX					:= ./gnu-efi
EFI_INCLUDE			:= $(PREFIX)/inc
# Installation gnu-efi 3.0 locale
# EFI_INCLUDE			:= /usr/local/include/efi

EFI_INCLUDES 		:= -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) \
		-I$(EFI_INCLUDE)/protocol -I$(INCLUDE_DIR)

EFI_PATH 				:= $(PREFIX)/x86_64
# Installation gnu-efi 3.0 locale
# EFI_PATH 				:= /usr/local/lib

LIB_GCC	 				:= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS 				:= -lefi -lgnuefi $(LIB_GCC)
EFI_LIBS_INCLUDE := -L$(EFI_PATH)/lib -L$(EFI_PATH)/gnuefi

EFI_CRT_OBJS 		:= $(EFI_PATH)/gnuefi/crt0-efi-$(ARCH).o
EFI_LDS 				:= $(PREFIX)/gnuefi/elf_x86_64_efi.lds

CC_FLAGS_ALL		:= -g -Wall -Werror -Werror -fno-stack-protector \
		-fno-strict-aliasing -fshort-wchar $(EFI_INCLUDES) -fno-builtin -fPIC -fPIE -O0

ifeq ($(ARCH),x86_64)
	CC_FLAGS_ALL	+= -DEFI_FUNCTION_WRAPPER
endif

LD_FLAGS_SCRIPT = -T $(EFI_LDS)

LD_FLAGS_ALL		= -nostdlib $(LD_FLAGS_SCRIPT) -shared -Bsymbolic $(EFI_LIBS_INCLUDE) \
		$(EFI_CRT_OBJS) -znocombreloc -fPIC --no-undefined -fPIE

define SRC_2_OBJ
  $(foreach src,$(1),$(patsubst sources/%,$(BUILD_DIR)/%,$(src)))
endef

define SRC_2_BIN
  $(foreach src,$(1),$(patsubst sources/%,$(BINARY_DIR)/%,$(src)))
endef

all: targets
# make -C test

# Overriden in rules.mk
TARGETS :=
OBJECTS :=

dir	:= sources
include	$(dir)/rules.mk

$(BUILD_DIR)/%.o: sources/%.s
	@echo "[CC] $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_TARGET) -o $@ -c $<

$(BUILD_DIR)/%.o: sources/%.c
	@echo "[CC] $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_TARGET) -o $@ -c $<

$(BINARY_DIR)/%.efi: $(BINARY_DIR)/%.elf
	@echo "[OC] $@"
#	@LANG=en objcopy -j .padding_begin -j .text -j .sdata -j .data \
#		-j .padding_begin -j .text -j .sdata -j .data -j .symtab \
#		-j .rel -j .reloc -j .padding_end \
#		$< $<.stripped \
#		2>&1 | grep -v 'warning: empty loadable segment detected' || true
#	@../linux/app/binary/elf2efi/elf2efi $<.stripped $@
	@objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .padding_end \
		$(OBJCPY_FLAGS_TARGET) \
	  $< $@

$(BINARY_DIR)/%.elf:
	@echo "[LD] $@"
	@mkdir -p $(dir $@)
	@$(LD) $(LD_FLAGS_ALL) $(LD_OBJECTS) -o $@ $(EFI_LIBS_TARGET) $(EFI_LIBS)

$(BINARY_DIR)/%.a:
	@echo "[AR] $@"
	@mkdir -p $(dir $@)
	@$(AR) rc $@ $(LD_OBJECTS)

targets: $(patsubst sources/%, $(BINARY_DIR)/%, $(TARGETS))

clean:
	@rm -f $(TARGETS) $(OBJECTS)

info:
	@echo Targets [$(TARGETS)]
	@echo Objects [$(OBJECTS)]

usb: all mount $(patsubst $(BINARY_DIR)/%, /mnt/EFI/%, $(TARGETS)) shell umount

#/mnt/kvm.ko: ../muse/linux-4.1.6/arch/x86/kvm/kvm.ko
#	sudo cp $^ $@
#
#/mnt/kvm-intel.ko: ../muse/linux-4.1.6/arch/x86/kvm/kvm-intel.ko
#	sudo cp $^ $@
#
#../muse/linux-4.1.6/arch/x86/kvm/kvm.ko:
#	make -C ../muse/linux-4.1.6
#
#../muse/linux-4.1.6/arch/x86/kvm/kvm-intel.ko:
#	make -C ../muse/linux-4.1.6

shell:
	sudo cp sources/shell_scripts/*.nsh /mnt

mount:
	sudo mount $(USB) /mnt

umount:
	sudo umount /mnt

/mnt/EFI/%: $(BINARY_DIR)/%
	@echo "[CP]    $^ -> $@"
	@sudo mkdir -p $(dir $@)
	@sudo cp $< $@

vmware: all vmware-mount $(patsubst $(BINARY_DIR)/%, /mnt/EFI/%, $(TARGETS)) \
	shell vmware-umount

vmware-mount:
	sudo vmware-mount vmware/hv.vmdk /mnt

vmware-umount:
	sudo vmware-mount -x

vmware-prepare:
	sudo /etc/init.d/vmware start
	sudo chown root:vmnet /dev/vmnet*
	sudo chmod g+rw /dev/vmnet*

vmware-compile:
	sudo vmware-modconfig --console --install-all

qemu: all launch

pre-launch:
	mkdir -p img/hda-contents/EFI
	cp -r $(BINARY_DIR)/* img/hda-contents/EFI
	cp -r sources/shell_scripts/* img/hda-contents
	cp img/hda-contents/startup{-qemu,}.nsh
	./run_qemu.sh

		# -bios /usr/share/ovmf/ovmf_x64.bin -m 8G
launch: pre-launch
	./run_qemu.sh
#	qemu-system-x86_64 \
#		-enable-kvm \
#		-cpu host \
#		-bios /usr/share/ovmf/x64/OVMF_CODE.fd \
#		-L . \
#		-m 8G \
#		-drive file=fat:rw:img/hda-contents,format=raw \
#		-cdrom img_arch/arch.iso \
#		-drive file=img_arch/vdisk.qcow2 \
#		-net nic,model=e1000 \
#		-net tap,ifname=tap99,script=no,downscript=no \
#		-debugcon file:debug.log \
#		-global isa-debugcon.iobase=0x402 \
#		-smp 1 \
#		-S -s
# 	qemu-system-x86_64 \
# 		-bios /usr/share/ovmf/ovmf_code_x64.bin \
# 		-L . \
# 		-m 8G \
# 		-drive file=fat:rw:img/hda-contents \
# 		-cdrom img_arch/arch.iso \
# 		-drive file=img_arch/vdisk.qcow2 -enable-kvm \
# 		-cpu host -net nic,model=e1000 \
# 		-net tap,ifname=tap99,script=no,downscript=no \
# 		-net user,vlan=1 -net nic,vlan=1,model=e1000 -smp 1 \
# 		-net dump -monitor stdio \
# 		-gdb tcp::9999
										
