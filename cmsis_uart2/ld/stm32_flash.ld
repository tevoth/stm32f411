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
    KEEP(*(.init))
    KEEP(*(.fini))
    KEEP(*(.preinit_array*))
    KEEP(*(.init_array*))
    KEEP(*(.fini_array*))
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  .data : 
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > SRAM AT>FLASH
  /* start of data in FLASH */
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
  end = _ebss;
  _end = _ebss;
}

