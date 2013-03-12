import struct
import mmap
import sys

# See packages!
import portio

portio.iopl(3)

if len(sys.argv) != 2:
	print("Syntax: %s outfile" % sys.argv[0])
	sys.exit(1)

outfile = sys.argv[1]
base_address = 0xf8000000
start_bus = 0
bus = 0
device = 0x1f
function = 0

wmem = open("/dev/mem", "r+b")
mem = open("/dev/mem", "rb")

wfileno = wmem.fileno()
fileno = mem.fileno()
offset = base_address + ((bus - start_bus) * (1 << 20)) + (device * (1 << 15)) + function * 4096
mmio = mmap.mmap(fileno, 4096, mmap.MAP_SHARED, mmap.PROT_READ, offset=offset)

ids = struct.unpack("<HH", mmio[0:4])
print("ids: %x %x" % ids)

# See 13.1 of Intel 7 Series Chipset and Intel C216 Series Chipset.
# Root Complex Register Block address
rcrba = struct.unpack("<I", mmio[0xf0:0xf4])[0]
print("rcrba: %x" % rcrba)

# We use 0x4000 because SPI goes from 0x3800 to 0x39ff
rcrb = mmap.mmap(fileno, 0x4000, mmap.MAP_SHARED, mmap.PROT_READ, offset=rcrba & 0xffffc000)
wrcrb = mmap.mmap(wfileno, 0x4000, mmap.MAP_SHARED, mmap.PROT_WRITE, offset=rcrba & 0xffffc000)

# See 10.1.43
gcs = struct.unpack("<I", rcrb[0x3410:0x3414])[0]
bbs = ((gcs >> 10) & 0x3)
bbs_type = ["LPC", "Res", "PCI", "SPI"][bbs]
print("gcs: %x" % gcs)
print("bbs: %x / %s" % (bbs, bbs_type))

def close():
	rcrb.close()
	mmio.close()
	mem.close()
	wmem.close()

if bbs != 3:
	print("access to BIOS without SPI not yet handled...")
	close()
	sys.exit(1)

# See 22.1:
#  The SPI Host Interface registers are memory-mapped in the RCRB (Root Complex
#  Register Block) Chipset Register Space with a base address (SPIBAR) of 3800h and are
#  located within the range of 3800h to 39FFh.

# Test "flash descriptor valid"
# TODO: test lockdown ???
hsfs = struct.unpack("<H", rcrb[0x3804:0x3806])[0]
hsfs_fdv = hsfs & (1 << 14)
print("hsfs: %x" % hsfs)
print("hsfs_fdv: %x" % hsfs_fdv)

if hsfs_fdv == 0:
	print("flash descriptor invalid!")
	close()
	sys.exit(1)

# See 22.1.9: region 1 is BIOS
frap = struct.unpack("<L", rcrb[0x3850:0x3854])[0]
print("frap: %x" % frap)
freg1 = struct.unpack("<L", rcrb[0x3858:0x385c])[0]
print("freg1: %x" % freg1)
bios_base = ((freg1 >> 0) & 0xfff) << 12
bios_limit = (((freg1 >> 16) & 0xfff) << 12) | 0xfff
print("BIOS base: %x  limit: %x" % (bios_base, bios_limit))


# See 22.1.2:
# FDONE, FCERR, AEL must be cleared (i.e. 0). If they equal 1, we can write this value for a reset.
#REGWRITE16(ICH9_REG_HSFS, REGREAD16(ICH9_REG_HSFS));
hsfs = rcrb[0x3804:0x3806]
wrcrb[0x3804:0x3806] = hsfs

fout = open(outfile, 'ab')

def close():
	rcrb.close()
	mmio.close()
	mem.close()
	wmem.close()
	fout.close()

index = bios_base
while index < bios_limit:
	print("processing: %x" % index)
	size_to_read = min(64, bios_limit - index + 1)

	# See 22.1.4
	faddr = rcrb[0x3808:0x380c]
	faddr = struct.unpack("<L", faddr)[0]
	faddr = (faddr & 0xfe000000) | index
	faddr = struct.pack("<L", faddr)
	wrcrb[0x3808:0x380c] = faddr

	hsfc = struct.unpack("<H", rcrb[0x3806:0x3808])[0]
	# See 22.1.3
	hsfc = hsfc & 0xc0f9	# Clear bits 2:1 (0x___9) => we want a read operation.
				# Clear bits 13:8 (0xc0__) => prepare byte count
	hsfc = hsfc | (((size_to_read - 1) << 8) & 0x3f00) | 1	# Add byte count and GO
	hsfc = struct.pack("<H", hsfc)
	wrcrb[0x3806:0x3808] = hsfc

	# Wait for the end of read...
	# See FDONE and FCERR in 21.1.2
	wait = True
	loop_limit = 32
	while wait:
		hsfs = struct.unpack("<H", rcrb[0x3804:0x3806])[0]
		wait = ((hsfs & 0x3) == 0)
		if wait:
			for i in range(10): pass
		loop_limit = loop_limit - 1
		if loop_limit == 0:
			print("Loop limit at %x" % index)
			close()
			sys.exit(1)
	if (hsfs & 0x2) == 0x2:
		print("Error at %x" % index)
		close()
		sys.exit(1)
	# Reset FDONE and FCERR
	hsfs = rcrb[0x3804:0x3806]
	wrcrb[0x3804:0x3806] = hsfs
		
	for i in range(int(size_to_read / 4)):
		# See 22.2.5
		data_from = 0x3810 + 4 * i
		data = rcrb[data_from:(data_from + 4)]
		fout.write(data)
	# It is impossible to replace the previous loop by:
	#    fout.write(rcrb[0x3810:(0x3810 + size_to_read)])
	# aybe we can't read several register at the same time!

	index = index + 64

close()
sys.exit(0)
