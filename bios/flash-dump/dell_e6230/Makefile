bios:
	dmidecode -s system-product-name | { read value; test "$$value" = "Latitude E6230"; }
	python print.py bios
