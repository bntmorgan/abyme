#!/usr/bin/env python3

import sys, os

#Magic

magic = 0x1badb002
cursor = 0
offset = 0
debug = 0

##
## main
##
if len(sys.argv) != 2:
    print ("usage: %s <config_file_name>" % sys.argv[0])
    sys.exit(1)

with open(sys.argv[1],"rb") as f:
  byte = f.read(1)
  while byte:
    # Check the magic noumba
    if byte[0] == ((magic >> ((8 * cursor)) & 0xff)):
      if debug:
        print ("Found %02x, cursor is 0x%08x" % ((magic >> ((8 * cursor)) & 0xff), cursor))
      cursor += 1
    else:
      cursor = 0
    if cursor == 4:
      print("Magic noumba found at offset 0x%08x" % (offset - 3))
      sys.exit(0)
    byte = f.read(1)
    offset += 1
f.close()
print("Magic noumba NOT found")
sys.exit(1)
