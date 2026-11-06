ENTRY(start)

KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;

SECTIONS {
  /* 1. Multiboot2 header and early 32-bit bootstrap code at physical 1 MiB */
  . = 1M;

  .boot : {
      KEEP(*(.multiboot_header))
      *(.boot.text)
      *(.boot.rodata)
      *(.boot.data)
      *(.boot.bss)
  }

  /* 2. Higher-half kernel, starting on a fresh page */
  . += KERNEL_VIRT_BASE;
  . = ALIGN(4096);
  _kernel_start = .;

  /* RX */
  _text_start = .;
  .text : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
      *(.text)
      *(.text.*)
  }
  . = ALIGN(4096);
  _text_end = .;

  /* R, NX */
  _rodata_start = .;
  .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
      *(.rodata)
      *(.rodata.*)
  }
  . = ALIGN(4096);
  _rodata_end = .;

  /* RW, NX */
  _data_start = .;
  .data : AT(ADDR(.data) - KERNEL_VIRT_BASE) {
      *(.data)
      *(.data.*)
  }

  .bss : AT(ADDR(.bss) - KERNEL_VIRT_BASE) {
      *(.bss)
      *(.bss.*)
      *(COMMON)
  }
  . = ALIGN(4096);
  _data_end = .;
  _kernel_end = .;

  /* Physical addresses for PMM frame reservation */
  _kernel_phys_start = _kernel_start - KERNEL_VIRT_BASE;
  _kernel_phys_end   = _kernel_end - KERNEL_VIRT_BASE;

  /DISCARD/ : {
      *(.comment)
      *(.eh_frame)
  }
}
