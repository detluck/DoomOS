section .multiboot_header

; --- Multiboot 1 Header (Enables direct QEMU -kernel booting) ---
align 4
multiboot1_header:
    dd 0x1BADB002                   ; Multiboot 1 magic number
    dd 0x00000003                   ; Flags: align 4KB page boundaries + provide memory map
    dd -(0x1BADB002 + 0x00000003)   ; Checksum

; --- Multiboot 2 Header (Enables GRUB 2 multiboot2 booting) ---
align 8
header_start:
    dd 0xe85250d6                   ; Multiboot 2 magic number
    dd 0                            ; Architecture 0 (32-bit protected mode)
    dd header_end - header_start    ; Header length
    ; Checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; End tag
    dw 0
    dw 0
    dd 8
header_end:
