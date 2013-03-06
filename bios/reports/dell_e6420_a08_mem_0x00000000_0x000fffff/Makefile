report.pdf: bios context formats Makefile sections
	../../tools/main.py > report.rst
	rst2pdf report.rst

bios:
	dmidecode -s bios-version        | { read value; test "$$value" = "A08"; }
	dmidecode -s system-product-name | { read value; test "$$value" = "Latitude E6420"; }
	dd if=/dev/mem of=bios bs=1 count=1048576
