#!/bin/bash

sudo ip tuntap add mode tap dev tap0
sudo ip addr add dev tap0 192.168.0.1/30
sudo ip link set tap0 up
sudo sysctl net.ipv4.ip_forward=1
