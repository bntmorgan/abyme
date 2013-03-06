import struct
import mmap
import sys

if len(sys.argv) != 2:
	print("Syntax: %s pcie-conf" % sys.argv[0])
	sys.exit(1)

f = open(sys.argv[1], "r")
spaces = f.readlines()
f.close()

mem = open("/dev/mem", "rb")
for space in spaces:
	entry = space.strip()
	entry = entry.split(" ")
	entry = [info.split("=") for info in entry]
	entry = dict(entry)

	start_bus = int(entry["start-bus"])
	end_bus = int(entry["end-bus"])
	base_address = int(entry["base-address"], 16)
	length = (end_bus - start_bus + 1) * 1024 * 1024
	fileno = mem.fileno()
	offset = base_address
	for bus in range(start_bus, end_bus + 1):
		for device in range(0, 32):
			for function in range(0, 8):
				mmio = mmap.mmap(fileno, 4096, mmap.MAP_SHARED, mmap.PROT_READ, offset=offset)
				ids = struct.unpack("<HH", mmio[0:4])
				if ids[0] != 0xffff or ids[1] != 0xffff:

					print("base %x start-bus %x bus %x device %x function %x vendor %x device %x" % ((base_address, start_bus, bus, device, function) + ids))
					mmio.close()
				offset = offset + 4096

offset = base_address
mmio = mmap.mmap(fileno, 4096, mmap.MAP_SHARED, mmap.PROT_READ, offset=offset)
memory_size = struct.unpack("<Q", mmio[0xa0:0xa8])[0]
print("memory size=%x" % memory_size)
mmio.close()

mem.close()
