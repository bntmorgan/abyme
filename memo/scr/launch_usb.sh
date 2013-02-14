#!/bin/bash

if test $# -lt 3; then
  echo "Usage: `basename $0` file_geometry file_image bochs_config files..."
  exit 1
fi

GEOMETRY=$1
IMAGE=$2
BOCHSCONFIG=$3
shift
shift
shift

# Load geometry
. $GEOMETRY

LOOP=`losetup -f`
losetup $LOOP $IMAGE

STARTSECTOR=`sfdisk -d $LOOP | grep "Id=83" | sed 's/^.*start=//;s/,.*$//;s/ *//'`
STARTBYTE=`expr $STARTSECTOR \* $BYTESPERSECTOR`
losetup -d $LOOP

losetup $LOOP $IMAGE -o $STARTBYTE
mount $LOOP /mnt

while test $# -ne 0; do
  echo "copy $1"
  cp $1 /mnt
  shift
done
umount /mnt
losetup -d $LOOP

bochs -f $BOCHSCONFIG
echo 0
