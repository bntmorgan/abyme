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

  .reloc : ALIGN(0x1000) { *(.reloc) }

  .text : ALIGN(0x1000) {
    *crt0-efi-*.o(.text)
    build/*/uefi_vmm_supervisor/efi.o(.text)

    /* PADDING */
    FILL(0x90)
    . = ALIGN(0x1000);

/* === Protected zone begin === */
    _protected_begin = .;
    *(.text)
  }

  .data : ALIGN(0x1000)
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

  .dynamic : ALIGN(0x1000) { *(.dynamic) }

  .rela : ALIGN(0x1000)
  {
    *(.rela.data*)
    *(.rela.got)
    *(.rela.stab)
  }

  .dynsym : ALIGN(0x1000) { *(.dynsym) }

  .dynstr : ALIGN(0x1000) { *(.dynstr) }

  .ignored.reloc : ALIGN(0x1000) { *(.rela.reloc) }

  .padding_end :
  {
    FILL(0x90);
    . = ALIGN(0x1000);
    _protected_end = .;
    BYTE(0x90)
  }
/* === Protected zone end === */

  ImageEnd = .;
}
