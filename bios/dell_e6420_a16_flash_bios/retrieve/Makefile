here: step_e

all: step_a step_b step_c step_d step_e

OBJDUMPFLAGS16=-D -b binary -mi8086 -Maddr16,data16
OBJDUMPFLAGS32=-D -b binary -mi8086 -Maddr32,data32
step_e:
	cd e.disasm/src												; \
	cp ../../d.bios.rom ../out/d.bios.rom									; \
	./main.py > ../out/result										;\
	cd ..													;

step_d:
	echo '(8086, 1c4f) == QM67'								; \
	INFO=`cat c.pcie-list | grep 'vendor 8086 device 1c4f$$' | cut -d" " -f2,4,6,8,10`	; \
	cd d.print-bios										; \
	rm out/*										; \
	python3.2 src/print.py $$INFO out/rom							; \
	cp out/rom ../d.bios.rom								; \
	cd ..											;

step_c:
	cd c.pci-info				; \
	python src/list.py ../b.pcie > out/out	; \
	cp out/out ../c.pcie-list		;

step_b:
	cd b.mcfg 					; \
	python src/decode.py ../a.MCFG.acpi > out/out	; \
	cp out/out ../b.pcie				;

step_a:
	cd a.acpidump				; \
	gcc -o src/acpidump src/acpidump.c	; \
	cd out					; \
	../src/acpidump				; \
	cp MCFG.acpi-tbl-* ../../a.MCFG.acpi	; \
	cd ../..				;
