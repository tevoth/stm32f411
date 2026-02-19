/* Entry Point */
ENTRY(Reset_Handler)

/* Memory definitions */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Top of stack */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/* Optional policy numbers (don’t reserve anything by themselves) */
__max_heap_size  = 0x200;
__max_stack_size = 0x400;

SECTIONS
{
  /* Start placing output at beginning of FLASH */
  . = ORIGIN(FLASH);

  /* Vector table should be first in flash */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector_tbl*))
    . = ALIGN(4);
  } > FLASH

  /* Code + const data in FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;     /* end of FLASH image used as .data load address */
  } > FLASH

  /* Initialized data in SRAM, loaded from FLASH after _etext */
  .data : AT(_etext)
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > SRAM
  _sidata = LOADADDR(.data);

  /* Zero-initialized data in SRAM */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > SRAM
}
