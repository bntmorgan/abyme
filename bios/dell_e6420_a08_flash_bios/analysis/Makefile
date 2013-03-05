test:
	python3.2 ./disasm-to-pseudocode.py -o 0xffc00000 -s 0xfffffb57 -e 0xfffffd14 -b 32 -f d.bios.rom > o
	objdump -D -b binary --adjust-vma=0xffc00000 --start-address=0xfffffb57 --stop-address=0xfffffd14 -mi386 -Maddr32,data32 d.bios.rom

all: emulator

report:
	./main.py > out
	rst2pdf out

emulator:
	objdump -D -b binary --no-show-raw-insn --adjust-vma=0xffc00000 --start-address=0xfffffd8d --stop-address=0xfffffe62 -mi386 -Maddr32,data32 d.bios.rom
	python3.2 ./emulator32.py -o 0xffc00000 -s 0xfffffd8d -e fffffe61 -b 32 -f d.bios.rom
