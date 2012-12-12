launch_floppy: bin/empty_floppy.img
	make -C src/loader
	make -C src/vmm
	cp bin/empty_floppy.img bin/floppy.img
	scr/launch_floppy.sh bin/floppy.img cnf/bochsrc_floppy.txt src/vmm/vmm.bin src/loader/loader.bin

launch_usb: bin/empty_usb.img
	make -C src/loader
	make -C src/vmm
	cp bin/empty_usb.img bin/usb.img
	scr/launch_usb.sh cnf/geometry_usb bin/usb.img cnf/bochsrc_usb.txt src/vmm/vmm.bin src/loader/loader.bin

bin/empty_floppy.img: scr/generate_floppy.sh
	scr/generate_floppy.sh bin/empty_floppy.img

bin/empty_usb.img: cnf/geometry_usb scr/generate_usb.sh
	scr/generate_usb.sh cnf/geometry_usb bin/empty_usb.img

clean:
	make -C src/vmm clean
	make -C src/loader clean

mrproper: clean
	rm -f bin/empty_floppy.img bin/floppy.img
	rm -f bin/empty_usb.img bin/usb.img
