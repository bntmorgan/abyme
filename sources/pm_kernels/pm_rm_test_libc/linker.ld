ENTRY (loader)
/*
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
*/
SECTIONS
{
  . = 0x00007c00;

  .text :
  {
    *(.text)
  }

  .text :
  {
    kernel_start = .;
    INCLUDE "binary/rm_kernels/rm_test_libc/kernel.bytes" ;
    kernel_end = .;
  }

  .rodata :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data)
  }

  .bss :
  {
    sbss = .;
    *(COMMON)
    *(.bss)
    ebss = .;
  }

}
