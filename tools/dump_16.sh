#!/bin/sh
objdump -D -b binary -mi386 -Maddr16,data16 $1
