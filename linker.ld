ENTRY(start)

SECTIONS {
  /* Load kernel at 1 MiB physical address */
  . = 1M;

  /* Multiboot header */
  .boot : {
      KEEP(*(.multiboot_header))
  }

  /* Kernel executable code */
  .text : {
      *(.text)
  }

  /* Read-only data (strings, constants) */
  .rodata : {
      *(.rodata)
  }

  /* Initialized global/static variables */
  .data : {
      *(.data)
  }

  /* Uninitialized variables and the stack */
  .bss : {
      *(.bss)
  }
}
