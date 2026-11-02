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

  /* 2. Higher-half kernel code and data mapped at 0xFFFFFFFF80000000 */
  . += KERNEL_VIRT_BASE;
  _kernel_start = .;

  .text : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
      *(.text)
      *(.text.*)
  }

  .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
      *(.rodata)
      *(.rodata.*)
  }

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
  _kernel_end = .;

  /* Physical addresses for PMM frame reservation */
  _kernel_phys_start = _kernel_start - KERNEL_VIRT_BASE;
  _kernel_phys_end   = _kernel_end - KERNEL_VIRT_BASE;

  /DISCARD/ : {
      *(.comment)
      *(.eh_frame)
  }
}
