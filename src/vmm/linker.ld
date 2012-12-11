OUTPUT_FORMAT ("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH ("i386:x86-64")
ENTRY (start)

PHDRS
{
   load  PT_LOAD FLAGS (6);
   drop  PT_LOAD FLAGS (6);
}

SECTIONS
{
  . = 0x0;



   .info_hdr : { *(.info_hdr)   } : load
   .idt_jmp  : { *(.idt_jmp)    } : load
   .text     : { *(.text*)      } : load
   .rodata   : { *(.rodata)     } : load
   .got      : { *(.got)        } : load
   .data     : { *(.data*)      } : load
   .bss      : { *(.bss COMMON) } : load

  .text ALIGN (0x1000) :
  {
    trampoline_start = .;
    INCLUDE "trampoline_bytes.ld" ;
    trampoline_end = .;
  } : load

   .dynamic  : { *(.dynamic)    } : drop
   .relocs   : { *(.rel*)       } : drop
   .got.plt  : { *(.got.plt)    } : drop

   /DISCARD/ : { *(.interp .note* .eh_frame .indent .comment .dynsym .dynstr .hash) }


/*

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

  .text ALIGN (0x1000) :
  {
    trampoline_start = .;
    INCLUDE "trampoline_bytes.ld" ;
    trampoline_end = .;
  }
*/
  kernel_end = .;
}
