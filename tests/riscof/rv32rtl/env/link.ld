OUTPUT_ARCH( "riscv" )
ENTRY(rvtest_entry_point)

MEMBASE = 0;
MEMSIZE = 1716K;

MEMORY{
  imem : ORIGIN = MEMBASE,         LENGTH = MEMSIZE
  dmem : ORIGIN = MEMBASE+MEMSIZE, LENGTH = MEMSIZE
}

SECTIONS
{
  .text :
  {
    *(.text.init .text)
  } > imem

  .data :
  {
    *(.data)
    *(.data.string)
    *(.bss)
    _end = .;
  } > dmem
}

