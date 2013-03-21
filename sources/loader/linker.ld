ENTRY (loader)

SECTIONS
{
  . = 0x00100000;
  .data   ALIGN (0x1000) : { *(.data)    }

  .text   ALIGN (0x1000) : { *(.text)    }
  .rodata ALIGN (0x1000) : { *(.rodata*) }

  .bss :
  {
    sbss = .;
    *(COMMON)
    *(.bss)
    ebss = .;
  }
}
