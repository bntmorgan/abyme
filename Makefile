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

CC_FLAGS_ALL		:= -Wall -Werror -Werror -fno-stack-protector \
		-fno-strict-aliasing -fshort-wchar $(EFI_INCLUDES) -fno-builtin -fPIC -O0

ifeq ($(ARCH),x86_64)
	CC_FLAGS_ALL	+= -DEFI_FUNCTION_WRAPPER
endif

LD_FLAGS_ALL		:= -nostdlib -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) \
		$(EFI_CRT_OBJS) -znocombreloc -fPIC --no-undefined

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
	@objcopy -j .padding_begin -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .padding_end \
		$(OBJCPY_FLAGS_TARGET) \
	  $< $@
	@strip $@

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
	sudo mount $(USB) /mnt --rw

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

qemu: launch

pre-launch:
	mkdir -p img/hda-contents/EFI
	cp -r $(BINARY_DIR)/* img/hda-contents/EFI
	cp -r sources/shell_scripts/* img/hda-contents
	./run_qemu.sh

launch: pre-launch
	qemu-system-x86_64 -bios /usr/share/ovmf/ovmf_x64.bin -m 16G \
		           -hda fat:img/hda-contents -cpu host -enable-kvm \
			   -net nic,model=e1000 \
			   -net tap,ifname=tap0,script=no,downscript=no 
	sudo /etc/qemu-ifdown tap0

qemu_clean:
	sudo netctl disable bridge
	sudo netctl stop bridge
	sudo modprobe -r tun
	sudo sysctl net.ipv4.ip_forward=0
