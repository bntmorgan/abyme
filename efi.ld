/* Same as elf_x86_64_fbsd_efi.lds, except for OUTPUT_FORMAT below - KEEP IN SYNC */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;
  .hash : { *(.hash) }	/* this MUST come first! */
  . = ALIGN(0x1000);
  ImageStart = .;
  .eh_frame : { *(.eh_frame) }
  .padding_begin : ALIGN(0x1000) {
   _padding_begin_a = .;
   BYTE(0x90);
   _padding_begin_b = . + 0x200000;
  }
  . = ALIGN(0x1000);
  .text : { *(.text) }
  . = ALIGN(4096);
  .reloc : { *(.reloc) }
  . = ALIGN(4096);
  .data :
  {
   *(.rodata*)
   *(.got.plt)
   *(.got)
   *(.data*)
   *(.sdata)
   /* the EFI loader doesn't seem to like a .bss section, so we stick
      it all into .data: */
   *(.sbss)
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
   *(.rel.local)
  }
  . = ALIGN(4096);
  .dynamic  : { *(.dynamic) }
  . = ALIGN(4096);
  .rela :
  {
    *(.rela.data*)
    *(.rela.got)
    *(.rela.stab)
  }
  . = ALIGN(4096);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(4096);
  .ignored.reloc : { *(.rela.reloc) }
  .padding_end :ALIGN(0x1000) {
   _padding_end_a = .;
   BYTE(0x90);
   _padding_end_b = . + 0x200000;
  }
}
