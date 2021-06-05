ENTRY (kernel_start)

SECTIONS
{
  . = 0x10000000;
  /* Dirty ass useless hack to be compatible with common/debug.c */
  ImageBase = .;
  .text   ALIGN (0x1000) : {
    *(.start)
    *(.text)
  }
  .data   ALIGN (0x1000) : {
    /* Dirty ass useless hack to be compatible with common/debug.c */
    _data = .;
    *(.data)
  }
  .rodata ALIGN (0x1000) : { *(.rodata*) }
  .bss : ALIGN (0x1000)
  {
    sbss = .;
    /* Force page tables to be at the beginning the aligned .bss section */
    *(.bss.zizi)
    *(COMMON)
    *(.bss)
    ebss = .;
  }
}
