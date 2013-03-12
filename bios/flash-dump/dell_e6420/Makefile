bios:
	dmidecode -s system-product-name | { read value; test "$$value" = "Latitude E6420"; }
	python print.py bios
