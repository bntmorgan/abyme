launch: bin/empty_floppy.img
	make -C src/loader
	make -C src/vmm
	#cp bin/empty_usb.img bin/usb.img
	cp bin/empty_floppy.img bin/floppy.img
	#scr/launch.sh cnf/geometry bin/usb.img cnf/bochsrc.txt src/vmm/vmm.bin src/loader/loader.bin
	scr/launch_floppy.sh bin/floppy.img cnf/bochsrc.txt src/vmm/vmm.bin src/loader/loader.bin

bin/empty_floppy.img: scr/generate_floppy.sh
	scr/generate_floppy.sh bin/empty_floppy.img

bin/empty_usb.img: cnf/geometry scr/generate.sh
	scr/generate.sh cnf/geometry bin/empty_usb.img

clean:
	make -C src/vmm clean
	make -C src/loader clean

mrproper: clean
	rm -f bin/empty_floppy.img bin/floppy.img
	rm -f bin/empty_usb.img bin/usb.img
