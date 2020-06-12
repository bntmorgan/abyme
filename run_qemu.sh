#!/bin/bash

QEMU=qemu-system-x86_64

which $QEMU
if test $? -ne 0
then
  echo Please install qemu or alias qemu-system-x86_64
  exit 1
fi

which brctl
if test $? -ne 0
then
  echo Please install brctl command via bridge-utils package
  exit 1
fi

nobrfilter(){
  # Deactivate bridge filtering
  sudo modprobe br_netfilter
  sudo sysctl net.bridge.bridge-nf-call-arptables=0
  sudo sysctl net.bridge.bridge-nf-call-iptables=0
  sudo sysctl net.bridge.bridge-nf-call-ip6tables=0
}

# $1 name of the bridge to create
brcreate(){
  sudo ip link add name $1 type bridge
  sudo ip link set $1 up
}

# $1 name of the tap to create
tapcreate(){
  echo create the interface $1 and add it to $2 bridge

  sudo ip tuntap add mode tap user `whoami` name $1
  sudo ip link set up dev $1

  # Connect to a bridge if required
  if test $# -eq 2
  then
    sudo brctl addif $2 $1
  fi
}

# $1 is the name of the interface to check
# $2 is the name of the interface creation function
# $3 is the argument of the interface creation function
createif(){
  echo does $1 exists already ?

  ip a | grep ": $1:"

  if test ! $? -eq 0
  then
    echo create the interface
    $2 $1 $3
  fi
}

# Create hypervisor tap interface
createif tap-hv tapcreate

# Deactivate bridge filtering
nobrfilter

# Set ip addresses
sudo ip addr add dev tap-hv 192.168.0.1/30
sudo ip link set tap-hv up

OVMF=./edk2/Build/Ovmf3264/DEBUG_GCC5/FV/OVMF_CODE.fd
VARS=./OVMF_VARS.fd

# -serial mon:stdio \
# -nographic \


BIOS="-bios /usr/share/ovmf/x64/OVMF_CODE.fd"
#BIOS=" \
#  -global driver=cfi.pflash01,property=secure,value=on \
#  -drive file=$OVMF,if=pflash,format=raw,unit=0,readonly=on \
#  -drive file=$VARS,if=pflash,format=raw,unit=1

#MACHINE=
MACHINE="-cpu host -enable-kvm"
#MACHINE="-machine q35,smm=on,accel=tcg"

$QEMU \
  $MACHINE \
  -m 4G \
  -smp 4,sockets=1,cores=4,threads=1 \
  -chardev pty,id=charserial1 \
  -device isa-serial,chardev=charserial1,id=serial1 \
  -netdev tap,id=net0,ifname=tap-hv,script=no,downscript=no \
  -device e1000,netdev=net0 \
  -drive file=fat:rw:img/hda-contents,format=raw \
  -cdrom img_arch/arch.iso \
  $BIOS \
  -drive file=img_arch/vdisk.qcow2 \
  -debugcon file:debug.log \
  -global isa-debugcon.iobase=0x402 \
  -s -S
