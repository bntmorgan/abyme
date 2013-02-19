#! /usr/bin/env python

import sys, struct

def read_msr(idx):
    file = open("/dev/cpu/0/msr", "rb" )
    file.seek( idx )
    eax, edx = struct.unpack( "@II", file.read(8) )
    file.close()
    return eax,edx
#endef


#
# Main
#
eax, edx = read_msr( 0x482 )

print "Primary proc based controls available"
print "\tEAX =", hex(eax), "EDX =", hex(edx)

if (edx & (1<<31)) != 0:
    eax, edx = read_msr( 0x48b )
    print "Secondary proc based controls available"
    print "\tEAX =", hex(eax), "EDX =", hex(edx)

    if (edx & (1<<1)) != 0:
        print "EPT available"
    if (edx & (1<<2)) != 0:
        print "GDT|IDT|LDT exiting available"
    if (edx & (1<<5)) != 0:
        print "VPID EPT available"
    if (edx & (1<<14)) != 0:
        print "shadow VMCS available"
else:
    print "No VMX advanced features"
