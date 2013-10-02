OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
  . = 0;
  image_base = .;
  . = ALIGN(0x1000);
  .eh_frame : { *(.eh_frame) }
  .padding_begin : ALIGN(0x1000) {
    _padding_begin_a = .;
    BYTE(0x90);
    _padding_begin_b = . + 0x200000;
  }
  . = ALIGN(0x1000);
  .text : { *(.text) }
  . = ALIGN(0x1000);
  .reloc : { *(.reloc) }
  . = ALIGN(0x1000);
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
  }
  . = ALIGN(0x1000);
  .dynamic : { *(.dynamic) }
  . = ALIGN(0x1000);
  .rela :
  {
    *(.rela.data*)
    *(.rela.got)
    *(.rela.stab)
  }
  . = ALIGN(0x1000);
  .dynsym : { *(.dynsym) }
  . = ALIGN(0x1000);
  .dynstr : { *(.dynstr) }

  .padding_end :ALIGN(0x1000) {
    _padding_end_a = .;
    BYTE(0x90);
    _padding_end_b = . + 0x200000;
  }
}
