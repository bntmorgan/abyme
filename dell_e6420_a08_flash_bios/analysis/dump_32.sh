#!/bin/sh
objdump -D -b binary -mi386 -Maddr32,data32 $1
