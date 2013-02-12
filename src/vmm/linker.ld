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

  .text ALIGN (0x1000) :
  {
    pepin_start = .;
    INCLUDE "pepin_bytes.ld" ;
    pepin_end = .;
  } : load

  .text ALIGN (0x1000) :
  {
    trampoline_start = .;
    INCLUDE "trampoline_bytes.ld" ;
    trampoline_end = .;
  } : load

  kernel_end = .;
}
