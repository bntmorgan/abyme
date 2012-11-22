launch:
	make -C src/loader
	make -C src/vmm
	cp bin/empty_usb.img bin/usb.img
	scr/launch.sh cnf/geometry bin/usb.img cnf/bochsrc.txt src/vmm/vmm.bin src/loader/loader.bin

bin/empty_usb.img: cnf/geometry scr/generate.sh
	scr/generate.sh cnf/geometry bin/empty_usb.img

clean:
	make -C src/vmm clean
	make -C src/loader clean

