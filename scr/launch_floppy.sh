#!/bin/bash

if test $# -lt 2; then
  echo "Usage: `basename $0` file_image bochs_config files..."
  exit 1
fi

IMAGE=$1
BOCHSCONFIG=$2
shift
shift

LOOP=`losetup -f`
losetup $LOOP $IMAGE
mount $LOOP /mnt

while test $# -ne 0; do
  echo "copy $1"
  cp $1 /mnt
  shift
done

umount /mnt
losetup -d $LOOP

bochs -f $BOCHSCONFIG
