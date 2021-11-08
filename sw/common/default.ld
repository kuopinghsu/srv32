OUTPUT_ARCH( "riscv" )
ENTRY(_start)
_start = 0;

MEMORY
{
  imem : ORIGIN = 0x00000000,       LENGTH = 128K
  dmem : ORIGIN = 0x00000000 +128K, LENGTH = 128K
}

SECTIONS
{
  /* text: test code section */
  .text :
  {
    _text_start = .;
    _start = .;
    *(.reset)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    *(.gnu.warning)
    _text_end = .;
  } > imem

  /* data segment */
  .data :
  {
    _data_start = .;
    __CTOR_LIST__ = .;
    ___CTOR_LIST__ = .;
    /* In 3.4.4, libgcc2.c actually required -1 here, rather than the count,
       as in ld documentation
    */
    LONG(-1)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;

    __DTOR_LIST__ = .;
    ___DTOR_LIST__ = .;
    LONG(-1)
    *(.dtors)
    LONG(0)
    __DTOR_END__ = .;

    KEEP (*(.gcc_except_table)) *(.gcc_except_table.*)

    *(.srodata .srodata.* .gnu.linkonce.r.*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.init*)
    *(.data .data.* .gnu.linkonce.d.*)
    KEEP (*(.gnu.linkonce.d.*personality*))
    . = ALIGN(32 / 8);
    _data_end = .;
  } > dmem

  .bss ALIGN(0x4) :
  {
    _bss_start = .;
    *(.dynbss)
    *(.sbss .sbss.* .gnu.linkonce.b.*)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    /* Align here to ensure that the .bss section occupies space up to
       _end.  Align after .bss to ensure correct alignment even if the
       .bss section disappears because there are no input sections.  */
    . = ALIGN(32 / 8);
    _bss_end = .;
  } > dmem

  .stack ALIGN(4) (NOLOAD) :
  {
    _end        = .;
  } > dmem

  PROVIDE(_stack = (128K * 2));

  /DISCARD/ : { *(.note.GNU-stack) }

}

