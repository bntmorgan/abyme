ENTRY (kernel_start)

SECTIONS
{
  . = 0x00100000;
  kernel_start = .;
  .text   ALIGN (0x1000) : { *(.text)    }
  .data   ALIGN (0x1000) : { *(.data)    }
  .rodata ALIGN (0x1000) : { *(.rodata*) }
  .bss : ALIGN (0x1000)
  {
    sbss = .;
    *(COMMON)
    *(.bss)
    ebss = .;
  }
}
