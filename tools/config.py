#!/usr/bin/env python

import itertools, sys, curses, os, pprint

##
## Ordered Dictionnary
##  - keys ordering is kept at insertion
##  - you can also access them by index (if already exists)
##
class Ordict:
    def __init__(self):
        self.dico = {}
        self.list = []

    def __getitem__(self, key):
        if type(key) == int:
            key = self.list[key]
        return self.dico[key]

    def __setitem__(self, key, val):
        if type(key) == int:
            if key >= len(self.list):
                raise ValueError
            key = self.list[key]
        else:
            if not key in self.list:
                self.list.append(key)
        self.dico[key] = val

    def get(self, k, dft=None):
        return self.dico.get(k,dft)

    def empty(self):
        return len(self.list) == 0

    def has_key(self, k):
        return self.dico.has_key(k)

    def __iter__(self):
        return self.iterkeys()

    def iterkeys(self):
        return (k for k in self.list)

    def keys(self):
        return [k for k in self.iterkeys()]

    def itervalues(self):
        return (self.dico[k] for k in self.list)

    def values(self):
        return [v for v in self.itervalues()]

    def iteritems(self):
        return ((k,self.dico[k]) for k in self.list)

    def items(self):
        return [(k,v) for k,v in self.iteritems()]

##
## Config
##
class Config(Ordict):
    def __init__(self, fname):
        Ordict.__init__(self)
        self.files = {
          "default":"tools/config.default",
          "syslinux.top":"tools/syslinux.top.cfg"
        }
        self.read(fname)

    def read(self, fname):
        try:
            self.files["config"] = fname
            fd = open(fname)
        except:
            try:
                fd = open(self.files["default"])
                print "default"
            except:
                print "failed to open config"
                sys.exit(1)

        for l in (ln.strip() for ln in fd):
            if l != '':
                k,v=l.split('=')
                self.__setitem__(k,v)


    def write(self):
        try:
            self.__write()
        except:
            print "failed to write config"
            sys.exit(1)

    def __write(self):
        fc = open(self.files["config"],"w")
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
        
        # Save the changes
        for t in self.items():
            fc.write("%s=%s\n" % (t[0],t[1]))
        
        
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
        fc.close()
        fsc.close()

##
## Menu
##
class Menu:
    def __init__(self, fconf):
        self.config = Config(fconf)
        self.file = "tools/config.menu"
        self.cancel_keys = (ord('q'), curses.KEY_BACKSPACE)
        self.active_keys = (ord('\n'), ord(' '))
        self.colors = { 'title': (1, curses.COLOR_CYAN, curses.COLOR_BLACK),
                        'field': (2, curses.COLOR_WHITE, curses.COLOR_BLACK),
                        'selec': (3, curses.COLOR_WHITE, curses.COLOR_RED),
                        'tips' : (4, curses.COLOR_GREEN, curses.COLOR_BLACK),
                        }
        self.node_types = {"menu":MenuNode,
                           "input":InputNode,
                           "choice":ChoiceNode,
                           "toggle":ToggleNode
                           }
        self.nodes = {}

        try:
            self.__render_init()
            self.__parse()
        except:
            self.__render_exit()
            print "failed to read menu"
            raise
            sys.exit(1)

    def __parse(self):
        fd = open(self.file)
        blocks = []
        block = Ordict()

        for l in (ln.strip() for ln in fd):
            if l != '':
                tag,block[tag]=map(lambda x: x.strip(), l.split(' ',1))
            else:
                blocks.append(block)
                block = Ordict()

        if not block.empty():
            blocks.append(block)

        fd.close()
        return self.__parse_blocks(blocks)

    def __parse_blocks(self, blocks):
        try:
            for b in blocks:
                tag = b[0]
                cls = self.node_types[b.keys()[0]]
                self.nodes[tag] = cls(self, tag, b)
            self.nodes["main"].parse()
        except:
            print "invalid menu"
            sys.exit(1)

    def __render_init(self):
        self.screen = curses.initscr()
        curses.noecho()
        curses.cbreak()
        curses.curs_set(0)
        curses.start_color()
        for c in self.colors.itervalues():
            curses.init_pair(*c)
        self.screen.keypad(1)

    def __render_exit(self):
        self.screen.keypad(0)
        curses.nocbreak()
        curses.echo()
        curses.endwin()

    def render(self):
        try:
            self.nodes["main"].show()
        finally:
            self.__render_exit()

    def interact(self):
        self.render()
        self.commit()

    def commit(self):
        self.config.write()

##
## Nodes
##
class Redisplay:
    basta = 0
    one   = 1
    pad   = 2
    full  = 3
    reset = 4

class Node:
    def __init__(self, root, tag, dico):
        self.root = root
        self.tag = tag
        self.title = dico["title"]
        self.data = dico.get("data")
        self.name, self.desc = map(lambda x: x.strip(), dico.get("desc",",").split(','))

    def _parse(self, data_needed=True):
        if data_needed and self.data == None:
            print self.tag,"node missing data field"
            raise KeyError

        self._x_parse()

        if hasattr(self, "entries"):
            self.len = len(self.entries)
            self.pad = curses.newpad(self.len, 100)
            self.redisplay = Redisplay.reset

    def __rst_display(self):
        self.ml, self.mc = map(lambda x:x-1,self.root.screen.getmaxyx())

        self.pads = 2
        self.pade = self.ml - 1
        self.padl = self.pade - self.pads + 1
        self.padc = self.mc

        self.cur = 0
        self.cs  = 0
        self.ce  = self.padl - 1

        self.__rst_pad()
        self.__hdr_display()
        self.do_scroll = True

    def __rst_setup(self):
        for n in self.root.nodes.itervalues():
            n.redisplay = Redisplay.reset

    def __rst_pad(self):
        lst = range(self.len)
        lst.remove(self.cur)
        self.display(self.cur, curses.color_pair(self.root.colors['selec'][0]))
        for i in lst:
            self.display(i, curses.color_pair(self.root.colors['field'][0]))

    def __upd_display(self, op):
        self.display(self.cur, curses.color_pair(self.root.colors['field'][0]))
        self.cur += op
        self.display(self.cur, curses.color_pair(self.root.colors['selec'][0]))

    def set_redisplay(self, how):
        if self.redisplay != Redisplay.reset:
            self.redisplay = how

    def __wait(self):
        self.redisplay = None
        key = self.root.screen.getch()
        if key == curses.KEY_RESIZE:
            self.__rst_setup()

        elif key in self.root.cancel_keys:
            self.redisplay = Redisplay.basta

        elif key == curses.KEY_UP and self.cur > 0:
            if self.cur == self.cs:
                self.cs -= 1
                self.ce -= 1
                self.do_scroll = True
            self.__upd_display(-1)

        elif key == curses.KEY_DOWN and self.cur < self.len - 1:
            if self.cur == self.ce:
                self.cs += 1
                self.ce += 1
                self.do_scroll = True
            self.__upd_display(1)

        elif key in self.root.active_keys:
            self.validate()

    def __hdr_display(self):
        self.__erase()
        col = curses.color_pair(self.root.colors['title'][0])
        self.root.screen.addstr(0,5, self.title, col)

    def __erase(self):
        self.do_scroll = True
        self.root.screen.erase()
        self.root.screen.refresh()

    def __scrolling(self):
        self.do_scroll = False
        col = curses.color_pair(self.root.colors['tips'][0])
        if self.cs > 0:
            self.root.screen.addstr(1,0, "(+)", col)
        else:
            self.root.screen.addstr(1,0, "   ", curses.color_pair(0))

        if self.ce < self.len - 1:
            self.root.screen.addstr(self.ml,0, "(+)", col)
        else:
            self.root.screen.addstr(self.ml,0, "   ", curses.color_pair(0))

    def __refresh(self):
        if self.redisplay == Redisplay.one:
            self.display(self.cur, curses.color_pair(self.root.colors['selec'][0]))
        elif self.redisplay == Redisplay.reset:
            self.__rst_display()
        elif self.redisplay == Redisplay.pad:
            self.__rst_pad()
        elif self.redisplay == Redisplay.full:
            self.__hdr_display()

        if self.do_scroll:
            self.__scrolling()

        self.pad.refresh(self.cs,0, self.pads,1, self.pade,self.padc)

    def show(self):
        while self.redisplay != Redisplay.basta:
            self.__refresh()
            self.__wait()

        self.__erase()
        self.set_redisplay(Redisplay.full)

class MenuNode(Node):
    def __init__(self, root, tag, dico):
        Node.__init__(self, root, tag, dico)

    def parse(self):
        self._parse()

    def _x_parse(self):
        self.entries = []
        for e in self.data.split(','):
            tag = e.strip()
            self.entries.append(self.root.nodes[tag])
            self.root.nodes[tag].parse()

    def display(self, i, col):
        self.pad.addstr(i, 0, self.entries[i].name, col)
        self.pad.addstr(i,15, self.entries[i].desc, col)

    def validate(self):
        self.entries[self.cur].show()
        self.set_redisplay(Redisplay.full)

class ChoiceNode(Node):
    def __init__(self, root, tag, dico):
        Node.__init__(self, root, tag, dico)

    def parse(self):
        self._parse()

    def _x_parse(self):
        split = self.data.split(',')
        self.choice = split[0]
        self.entries = []
        for e in split[1:]:
            try:
                nm,val = e.split(':')
            except ValueError:
                nm = e
                val = ""
            self.entries.append((nm.strip(),val.strip()))

    def display(self, i, col):
        nm, val = self.entries[i]
        if self.root.config[self.choice] == nm:
            v = "x"
        else:
            v = " "
        self.pad.addstr(i, 0, "["+v+"] "+nm, col)
        self.pad.addstr(i,15, val, col)

    def validate(self):
        self.root.config[self.choice] = self.entries[self.cur][0]
        self.set_redisplay(Redisplay.pad)

class ToggleNode(Node):
    def __init__(self, root, tag, dico):
        Node.__init__(self, root, tag, dico)

    def parse(self):
        self._parse(False)

    def _x_parse(self):
        self.entries = []
        if self.data != None:
            for e in self.data.split(','):
                cfg,hlp = map(lambda x: x.strip(), e.split(':'))
                self.entries.append((cfg,hlp))
        try:
            for e in getattr(self.root.config, self.tag):
                self.entries.append((e,""))
        except:
            pass

    def display(self, i, col):
        cfg,hlp = self.entries[i]
        if self.root.config[cfg] == "yes":
            v = "x"
        else:
            v = " "
        self.pad.addstr(i, 0, "["+v+"] "+cfg, col)
        self.pad.addstr(i,25, hlp, col)

    def validate(self):
        cfg = self.entries[self.cur][0]
        if self.root.config[cfg] == "yes":
            self.root.config[cfg] = "no"
        else:
            self.root.config[cfg] = "yes"

        self.set_redisplay(Redisplay.one)

class InputNode(Node):
    def __init__(self, root, tag, dico):
        Node.__init__(self, root, tag, dico)

    def parse(self):
        self._parse(False)

    def _x_parse(self):
        self.input = self.data.strip()

    def show(self):
        while True:
            col = curses.color_pair(self.root.colors['field'][0])
            self.root.screen.erase()
            self.root.screen.addstr(1,2, self.title+" ["+
                                    self.root.config[self.input]+"]", col)
            self.root.screen.refresh()
            key = self.root.screen.getch()
            if key in self.root.cancel_keys:
                self.root.screen.erase()
                break
            elif key in self.root.active_keys:
                self.root.screen.erase()
                self.root.screen.addstr(1,2, self.title+": ", col)
                curses.echo()
                curses.curs_set(1)
                self.root.config[self.input] = self.root.screen.getstr()
                curses.curs_set(0)
                curses.noecho()

##
## main
##
if len(sys.argv) != 2:
    print "usage: %s <config_file_name>" % sys.argv[0]
    sys.exit(1)

Menu(sys.argv[1]).interact()