OUTPUT_FORMAT ("binary")
ENTRY (start)

SECTIONS
{
  . = 0x200000;

  .text ALIGN (0x1000) :
  {
    *(.text)
  }

  .bss :
  {
    *(.bss);
  }

  .data ALIGN (0x1000) :
  {
    *(.data)
  }

  .rodata ALIGN (0x1000) :
  {
    *(.rodata*)
  }

  kernel_end = .;
}
