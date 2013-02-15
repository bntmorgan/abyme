#!bin/bash
# Generated with $[config]$
mount $[device]$ $[mount_point]$

# Create boot folder
mkdir $[mount_point]$/$[boot_directory]$ -p

# Install syslinux
if [ ! -e $[mount_point]$/$[config_file]$ ]; then
  $[command]$ $[mount_point]$
  #Copying the syslinux binaries
  ${for b in bs :}$
    cp /usr/lib/syslinux/$[b.strip()]$ $[mount_point]$
  ${#end}$
fi

# Create loader folder
mkdir $[mount_point]$/$[boot_directory]$/loader -p
# Copy the loader
cp binary/loader/loader.elf32 $[mount_point]$/$[boot_directory]$/loader

# Create vmm folder
mkdir $[mount_point]$/$[boot_directory]$/vmm -p
# Copy the vmm
cp binary/loader/loader.elf32 $[mount_point]$/$[boot_directory]$/vmm

# List and copy files from bin/pm_kernels/<name>/kernel.bin to boot/pm_kernels/<name>/kernel.bin
${for k in pm_kernels :}$
  mkdir $[mount_point]$/$[boot_directory]$/pm_kernels/$[k[0]]$ -p
  cp binary/pm_kernels/$[k[0]]$/$[k[1]]$ $[mount_point]$/$[boot_directory]$/pm_kernels/$[k[0]]$
${#end}$

# List and copy files from binary/rm_kernels/<name>/kernel.bin to boot/rm_kernels/<name>/kernel.bin
${for k in rm_kernels :}$
  mkdir $[mount_point]$/$[boot_directory]$/rm_kernels/$[k[0]]$ -p
  cp binary/rm_kernels/$[k[0]]$/$[k[1]]$ $[mount_point]$/$[boot_directory]$/rm_kernels/$[k[0]]$
${#end}$

# Copy the syslinux configuration file
cp $[syslinux_config]$ $[mount_point]$/$[config_file]$

# Umount the key
umount $[mount_point]$
