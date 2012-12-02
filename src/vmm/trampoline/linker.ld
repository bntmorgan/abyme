ENTRY(start16);

SECTIONS
{
  . = 0x0;
  .text : AT(0x0)
  {
    _text = .;
    *(.text);
    _text_end = .;
  }
  .data :
  {
    _data = .;
    *(.bss);
    *(.bss*);
    *(.data);
    *(.rodata*);
    *(COMMON)
    _data_end = .;
  }
  .sig : AT(0x1fe)
  {
    SHORT(0xaa55);
  }
  .stack : AT(0x900)
  {
    . += 0x100;
  }
  /DISCARD/ :
  {
    *(.note*);
    *(.iplt*);
    *(.igot*);
    *(.rel*);
    *(.comment);
  }
}

