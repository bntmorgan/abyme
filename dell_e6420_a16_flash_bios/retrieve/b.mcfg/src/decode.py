import struct
import sys

if len(sys.argv) != 2:
	print("Syntax: %s MCFG.acpi" % sys.argv[0], file=sys.stderr)
	sys.exit(1)

f = open(sys.argv[1], 'rb')
d = f.read()
for i in range(44, len(d), 16):
	e = struct.unpack("<QHBBI", d[i:(i + 16)])
	print("entry=%d base-address=%x group=%d start-bus=%x end-bus=%d reserved=%x" % ((i, ) + e))
f.close()
