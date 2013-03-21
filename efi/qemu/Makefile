pre-launch:
	rm -rf roms/*
	cp packages/OVMF-X64-r11337-alpha.zip roms
	cd roms ; unzip OVMF-X64-r11337-alpha.zip
	rm roms/OVMF-X64-r11337-alpha.zip
	mv roms/CirrusLogic5446.rom roms/vgabios-cirrus.bin
	mv roms/OVMF.fd roms/bios.bin
	make -C sources
	cp sources/binary/efi/hello_world/hello_world.efi hda-contents

launch: pre-launch
	qemu-system-x86_64 -L roms -hda fat:hda-contents
