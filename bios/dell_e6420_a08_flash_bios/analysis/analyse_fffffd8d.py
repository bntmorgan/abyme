import struct 

f = open("d.bios.rom", "rb")
d = f.read()
f.close()

def mem(address, size=32):
  if size == 32:
    offset = address - 0xffc00000
    return struct.unpack("<I", d[offset:offset + 4])[0]
  if size == 8:
    offset = address - 0xffc00000
    return struct.unpack("<B", d[offset:offset + 1])[0]
  else:
    return None

def simulate():
  ebx = 0xffc20000
  edx = ebx
  eax = mem(ebx + 0x30) & 0xffff
  ebx = ebx + eax
  edx = edx + mem(edx + 0x20)

  ecx = 0
  esi = 0
  edi = 0
  print("X. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
  while True:
    print("A. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
    if mem(ebx) == 0xffffffff:
      print("out $0xe,$0x80")
      print("return 0")
      return
    ecx = 0x4
    esi = ebx
    edi = 0xfffffd65
  
    ok = True
    for i in range(ecx * 4):
      if mem(edi, 8) != mem(esi, 8):
        ok = False
        break
      #print("? %02x:  %02x %02x" % (ecx, mem(edi, 8), mem(edi, 8)))
      ecx = ecx - 1
      esi = esi + 1
      edi = edi + 1
 
    if ok:
      edi = ebx + (mem(ebx + 0x14) & 0xffffff)
      ebx = ebx + 0x18
      esi = ebx
      #print("A partir de maintenant, esi semble pointer vers la structure (et non ebx)")
      eax = 1
      #print("cpuid")
      eax = 0x000306a9
      ebx = 0x01100800
      ecx = 0x7fbae3ff
      edx = 0xbfebfbff
      ebx = eax

      ecx = 0x17
      eax = 0x000000
      edx = 0x100000

      edx = (edx >> 0x12) & 0xffffff07
      ecx = (ecx & 0xffffff00) | (edx & 0x000000ff)
      edx = (edx & 0xffffff00) | 0x1
      edx = (edx & 0xffffff00) | ((edx & 0x000000ff) << (ecx & 0x000000ff))
  
      print("B. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
      while True:
        print("C. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
        if esi >= edi:
          print("looped over all elements (list was full of elements)")
          print("return eax=0")
          return
        if mem(esi) == 0xffffffff:
          print("reached last elements (list is not full)")
          print("return eax=0")
          return
        # fffffe0d
        ecx = 0x800
        if mem(esi + 0x1c) != 0x0:
          ecx = mem(esi + 0x20)
        # fffffe1b
        print("D. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
        print("   ebx[%08x]==0xc(esi)[%08x] dl[%02x]==0x18(esi)[%02x]" % (ebx, mem(esi + 0xc), edx & 0xff, mem(esi + 0x18, 8)))
        print("   le champ 0x0c(esi) est compare au retour eax suite au cpuid(0x01) (version proc)")
        print("   le champ 0x18(esi) est compare au retour ebx suite au rdmsr(0x17)")
        if ebx == mem(esi + 0xc) and (edx & 0xff) != mem(esi + 0x18, 8):
          print("return eax=esi=%08x", esi)
          return
        ebp = mem(esi + 0x20)
        eax = mem(esi + 0x1c) + 0x30
        print("E. eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"% (eax, ebx, ecx, edx, esi, edi))
        # fffffe30
        if ebp > eax:
          # fffffe34
          print("TODO: check these affectations")
          ecx = mem(esi + eax)
          ebp = eax + esi + 0x14

          while ecx > 0:
            if ebx == mem(ebp) and (edx & 0xff) != mem(ebp + 0x4, 8):
              print("return eax=esi=%08x", esi)
              return
            ebp = ebp + 0xc
            ecx = ecx - 1
          ecx = mem(esi + 0x20)
        # fffffe4f
        ecx = (ecx + 0x1ff) & 0xfffffe00
        esi = esi + ecx
    eax = mem(ebx + 0x14) & 0xffffff
    ebx = (ebx + eax + 0x7) & 0xfffffff8
    if ebx >= edx:
      print("out $0xe,$0x80")
      print("return 0")
      return


simulate()
