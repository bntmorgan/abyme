#!/bin/bash

## Need : - bridge-utils (brctl)

## Start forwarding IPv4
echo 'Start forwarding'
FORWARD=`cat /proc/sys/net/ipv4/conf/all/forwarding`
if [ $FORWARD = 0 ]; then
	sudo sysctl net.ipv4.ip_forward=1
fi

# Load of the tun module
echo 'Load tun module'
TUN=/dev/net/tun
if [ -r $TUN ]; then
	echo '--Already have tun node'
else
	mknod /dev/net/tun c 10 200
fi
sudo chmod 666 /dev/net/tun
sudo modprobe tun

## Creation of the bridge br0
echo 'Creation of the bridge br0'
sudo cp ./kvm/bridge_config /etc/netctl/bridge
BRIDGE=`netctl is-enabled bridge`
if [ "${BRIDGE}" = "enabled" ]; then
	echo '--Already br0'
else
	echo '--Start bridge br0'
	sudo netctl start bridge
	sudo netctl enable bridge > /dev/null
fi

## Copy the two scripts for Qemu
echo 'Copy of qemu-ifup and qemu-ifdown'
sudo cp ./kvm/qemu-ifup /etc/qemu-ifup
sudo cp ./kvm/qemu-ifdown /etc/qemu-ifdown

echo 'Add tap interface'
TAP=`ip tuntap show`
if [ -z $TAP ]; then
	USERID=`whoami`
	echo '--Add tap0'
	sudo /usr/bin/ip tuntap add user $USERID mode tap
else
	echo '--Already tap0'
fi

sudo /etc/qemu-ifup tap0
#sudo ip tuntap add user jonathan mode tap
#make qemu
#sudo /etc/qemu-ifdown tap0
