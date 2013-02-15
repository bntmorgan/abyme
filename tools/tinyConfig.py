#!/usr/bin/env python

#import itertools, sys, curses, os, pprint
import sys, os
from template import render
from config import Config, Menu, Ordict

class TinyConfig(Config):
  def __init__(self, fname):
    Config.__init__(self, fname)

  def write(self):
    try:
      self.__write()
    except IOError as e:
      print "failed to write config" + e.strerror
      sys.exit(1)
    
  def __write(self):
    # Open files
    fs = open(self.files["config"] + ".sh","w")
    # Top syslinux generated configuration file
    fsc = open(self.files["config"] + ".syslinux.cfg","w")
    
    # Copy top syslinux file in syslinux config
    d = {}
    d["device"] = self.get("CONFIG_USB_DEVICE")
    d["mount_point"] = self.get("CONFIG_USB_MOUNT_POINT")
    d["boot_directory"] = self.get("CONFIG_USB_BOOT_DIRECTORY")
    d["command"] = self.get("CONFIG_USB_SYSLINUX_COMMAND")
    d["config_file"] = self.get("CONFIG_USB_SYSLINUX_CONFIG_FILE")
    bins = self.get("CONFIG_USB_SYSLINUX_BINS")
    d["bs"] = bins.rsplit(",")
    d["config_file"] = self.files['config']
    
    # List and copy files from bin/pm_kernels/<name>/kernel.bin to boot/pm_kernels/<name>/kernel.bin
    pm_kernels = []
    for dirname, dirnames, filenames in os.walk('binary/pm_kernels'):
      for kernel in dirnames:
        kernel_bin = os.listdir(dirname + '/' + kernel)[0]
        pm_kernels.append((kernel, kernel_bin))
    d["pm_kernels"] = pm_kernels


    # List and copy files from binary/rm_kernels/<name>/kernel.bin to boot/rm_kernels/<name>/kernel.bin
    rm_kernels = []
    for dirname, dirnames, filenames in os.walk('binary/rm_kernels'):
      for kernel in dirnames:
        kernel_bin = os.listdir(dirname + '/' + kernel)[0]
        rm_kernels.append((kernel, kernel_bin))
    d["rm_kernels"] = rm_kernels

    fs.write("%s" % render("tools/scriptshell.tpl", d))
    
    fs.close()
    fsc.close()

  def __write_old(self):
    # Call config write
    Config.write(self)
    # Open files
    fs = open(self.files["config"] + ".sh","w")
    # Top syslinux configuration file
    fst = open(self.files["syslinux.top"],"r")
    # Top syslinux generated configuration file
    fsc = open(self.files["config"] + ".syslinux.cfg","w")
    # Copy top syslinux file in syslinux config
    for l in (ln.strip() for ln in fst):
      if l != '':
        fsc.write(l+"\n");
    fst.close()
    
    # Get the config vars
    device = self.get("CONFIG_USB_DEVICE")
    mount_point = self.get("CONFIG_USB_MOUNT_POINT")
    boot_directory = self.get("CONFIG_USB_BOOT_DIRECTORY")
    command = self.get("CONFIG_USB_SYSLINUX_COMMAND")
    config_file = self.get("CONFIG_USB_SYSLINUX_CONFIG_FILE")
    bins = self.get("CONFIG_USB_SYSLINUX_BINS")

    # Generate the scriptshell
    fs.write("#!/bin/bash\n")
    fs.write("# Generated with %s configuration file :)\n" % self.files["config"])
    fs.write("mount %s %s\n" % (device, mount_point))

    # Installing Syslinux
    fs.write("if [ ! -e %s/%s ]; then\n" % (mount_point, config_file))
    fs.write("%s %s\n" % (command, mount_point))
    #Copying the syslinux files
    bs = bins.rsplit(",")
    for b in bs:
      fs.write("cp /usr/lib/syslinux/%s %s/\n" % (b.strip(), mount_point))
    fs.write("fi\n")

    # Create boot folder
    fs.write("mkdir %s/%s -p\n" % (mount_point, boot_directory))

    # Create loader folder
    fs.write("mkdir %s/%s/loader -p\n" % (mount_point, boot_directory))
    # Copy the loader
    fs.write("cp binary/loader/loader.elf32 %s/%s/loader \n" % (mount_point, boot_directory))
    
    # Create vmm folder
    fs.write("mkdir %s/%s/vmm -p\n" % (mount_point, boot_directory))
    # Copy the vmm
    fs.write("cp binary/vmm/vmm.elf32 %s/%s/vmm \n" % (mount_point, boot_directory))

    # List and copy files from bin/pm_kernels/<name>/kernel.bin to boot/pm_kernels/<name>/kernel.bin
    for dirname, dirnames, filenames in os.walk('binary/pm_kernels'):
      for subdirname in dirnames:
        kernel = os.listdir(dirname + '/' + subdirname)[0]
        fs.write("mkdir %s/%s/pm_kernels/%s -p\n" % (mount_point, boot_directory, subdirname))
        fs.write("cp binary/pm_kernels/%s/%s %s/%s/pm_kernels/%s\n" % (subdirname, kernel, mount_point, boot_directory, subdirname))
        # Syslinux entry
        fsc.write("label protected_%s_%s\n" % (subdirname, kernel))
        fsc.write("menu label %s %s without tinyvisor\n" % (subdirname, kernel))
        fsc.write("kernel mboot.c32\n")
        fsc.write("append %s/pm_kernels/%s/%s\n\n" % (boot_directory, subdirname, kernel))

    # List and copy files from binary/rm_kernels/<name>/kernel.bin to boot/rm_kernels/<name>/kernel.bin
    for dirname, dirnames, filenames in os.walk('binary/rm_kernels'):
      for subdirname in dirnames:
        kernel = os.listdir(dirname + '/' + subdirname)[0]
        fs.write("mkdir %s/%s/rm_kernels/%s -p\n" % (mount_point, boot_directory, subdirname))
        fs.write("cp binary/rm_kernels/%s/%s %s/%s/rm_kernels/%s\n" % (subdirname, kernel, mount_point, boot_directory, subdirname))
        # Syslinux entry
        fsc.write("label tinyvisor_%s_%s\n" % (subdirname, kernel))
        fsc.write("menu label %s %s with tinyvisor\n" % (subdirname, kernel))
        fsc.write("kernel mboot.c32\n")
        fsc.write("append %s/loader/loader.bin --- %s/vmm/vmm.bin --- %s/rm_kernels/%s/%s\n\n" % (boot_directory, boot_directory, boot_directory, subdirname, kernel))

    # Copy the syslinux configuration file
    fs.write("cp %s %s/%s\n" % (self.files["config"] + ".syslinux.cfg", mount_point, config_file))

    # Umount the key
    fs.write("umount %s\n" % (mount_point))

    fs.close()
    fsc.close()

##
## main
##
if len(sys.argv) != 2:
    print "usage: %s <config_file_name>" % sys.argv[0]
    sys.exit(1)

Menu(TinyConfig(sys.argv[1])).interact()
