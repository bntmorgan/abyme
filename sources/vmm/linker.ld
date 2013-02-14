OUTPUT_FORMAT ("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH ("i386:x86-64")
ENTRY (start)

PHDRS
{
  load PT_LOAD FLAGS (6);
}

SECTIONS
{
  . = 0x0;
  kernel_start = .;

  .text     : { *(.text*)      } : load
  .rodata   : { *(.rodata)     } : load
  .data     : { *(.data*)      } : load
  .bss      : { *(.bss COMMON) } : load

  kernel_end = .;
}
