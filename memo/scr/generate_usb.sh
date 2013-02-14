#!/bin/bash

if test $# -ne 2; then
  echo "Usage: `basename $0` file_geometry file_image"
  exit 1
fi

GEOMETRY=$1
IMAGE=$2

# Load geometry
. $GEOMETRY

BYTESPERCYLINDERS=`expr $HEADS \* $SECTORSPERTRACK \* $BYTESPERSECTOR`

dd if=/dev/zero of=${IMAGE} bs=${BYTESPERCYLINDERS}c count=${CYLINDERS}
echo "Size: `expr ${HEADS} \* ${CYLINDERS} \* ${SECTORSPERTRACK} \* 512` bytes"

LOOP=`losetup -f`
losetup $LOOP $IMAGE
echo ",,83,," | sfdisk $LOOP
sfdisk $LOOP -A 1
fdisk -lu $IMAGE

STARTSECTOR=`sfdisk -d $LOOP | grep "Id=83" | sed 's/^.*start=//;s/,.*$//;s/ *//'`
STARTBYTE=`expr $STARTSECTOR \* $BYTESPERSECTOR`
losetup -d $LOOP
losetup $LOOP $IMAGE -o $STARTBYTE

mke2fs $LOOP
mount $LOOP /mnt

dd bs=440 count=1 conv=notrunc if=/usr/lib/syslinux/mbr.bin of=$IMAGE
cp /usr/lib/syslinux/mboot.c32 /mnt/mboot.c32
cat > /mnt/syslinux.cfg << EOF
default sechyp
label sechyp
        kernel mboot.c32
        append loader.bin --- vmm.bin
EOF
extlinux --install -S $SECTORSPERTRACK /mnt

umount /mnt
losetup -d $LOOP
