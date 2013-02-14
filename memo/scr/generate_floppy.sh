#!/bin/bash

if test $# -ne 1; then
  echo "Usage: `basename $0` file_image"
  exit 1
fi

IMAGE=$1

dd if=/dev/zero of=${IMAGE} bs=1024 count=1440

LOOP=`losetup -f`
losetup $LOOP $IMAGE

mkfs $LOOP
mount $LOOP /mnt

cp /usr/lib/syslinux/mboot.c32 /mnt/mboot.c32
cp /usr/lib/syslinux/menu.c32 /mnt/menu.c32
cat > /mnt/syslinux.cfg << EOF
ui menu.c32
menu title Tinyvisor Boot Menu
prompt 0
timeout 1000

label sechyp
  menu label Tinyvisor - Security hypervisor
  kernel mboot.c32
  append loader.bin --- vmm.bin

label hd
  menu label Boot from next BIOS boot device
  localboot -1
EOF
extlinux --install /mnt

umount /mnt
losetup -d $LOOP
