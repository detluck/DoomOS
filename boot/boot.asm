global start
extern kernel_main

; Higher-half virtual base (-2 GiB)
KERNEL_VIRT_BASE equ 0xFFFFFFFF80000000

; ==============================================================================
; Early Bootstrap BSS (Physical memory ~1 MiB)
; ==============================================================================
section .boot.bss nobits alloc write
align 4096
p4_table:
    resb 4096
p3_table_low:
    resb 4096
p3_table_high:
    resb 4096
p3_table_direct:
    resb 4096
p2_table:
    resb 4096
boot_stack_bottom:
    resb 4096          ; 4 KiB temporary stack for 32-bit bootstrap
boot_stack_top:

; ==============================================================================
; Early Bootstrap Read-Only Data (Physical memory ~1 MiB)
; ==============================================================================
section .boot.rodata progbits alloc noexec
align 8
gdt64:
    dq 0                         ; Null descriptor
.code_segment equ $ - gdt64
    ; Executable, Readable, Code/Data, Present, Long Mode (64-bit)
    dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dd gdt64

; ==============================================================================
; Early Bootstrap Code (Physical memory ~1 MiB, executed in 32-bit mode)
; ==============================================================================
section .boot.text progbits alloc exec
bits 32
start:
    ; 1. Set up temporary stack in physical low memory
    mov esp, boot_stack_top

    ; 2. Preserve Multiboot bootloader arguments
    mov edi, eax        ; 1st argument: Multiboot magic
    mov esi, ebx        ; 2nd argument: Multiboot info physical address

    ; 3. Populate P2 table: Map first 1 GiB using 512 x 2 MiB huge pages
    mov ecx, 0
.map_p2:
    mov eax, 0x200000       ; 2 MiB
    mul ecx                 ; EAX = ecx * 2 MiB
    or eax, 0b10000011      ; Present + Writable + Huge (2 MiB)
    mov [p2_table + ecx * 8], eax
    mov dword [p2_table + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne .map_p2

    ; 4. Low Identity Map (Virtual 0x0000000000000000 - 0x000000003FFFFFFF)
    ; Link P4[0] -> p3_table_low
    mov eax, p3_table_low
    or eax, 0b11            ; Present + Writable
    mov [p4_table], eax
    mov dword [p4_table + 4], 0

    ; Link p3_table_low[0] -> p2_table
    mov eax, p2_table
    or eax, 0b11            ; Present + Writable
    mov [p3_table_low], eax
    mov dword [p3_table_low + 4], 0

    ; 5. Higher-Half Kernel Map (Virtual 0xFFFFFFFF80000000 - 0xFFFFFFFFBFFFFFFF)
    ; Index calculation for 0xFFFFFFFF80000000:
    ; PML4 index = 511 (0x1FF)
    ; PDPT index = 510 (0x1FE)
    ; Link P4[511] -> p3_table_high
    mov eax, p3_table_high
    or eax, 0b11            ; Present + Writable
    mov [p4_table + 511 * 8], eax
    mov dword [p4_table + 511 * 8 + 4], 0

    ; Link p3_table_high[510] -> p2_table
    mov eax, p2_table
    or eax, 0b11            ; Present + Writable
    mov [p3_table_high + 510 * 8], eax
    mov dword [p3_table_high + 510 * 8 + 4], 0

    ; 6. Linux Direct Physical Map (Virtual 0xFFFF800000000000 - 0xFFFF80003FFFFFFF)
    ; Index calculation for 0xFFFF800000000000:
    ; PML4 index = 256 (0x100)
    ; PDPT index = 0
    ; Link P4[256] -> p3_table_direct
    mov eax, p3_table_direct
    or eax, 0b11            ; Present + Writable
    mov [p4_table + 256 * 8], eax
    mov dword [p4_table + 256 * 8 + 4], 0

    ; Link p3_table_direct[0] -> p2_table
    mov eax, p2_table
    or eax, 0b11            ; Present + Writable
    mov [p3_table_direct], eax
    mov dword [p3_table_direct + 4], 0

    ; 7. Load CR3 with physical address of P4 table
    mov eax, p4_table
    mov cr3, eax

    ; 8. Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 9. Enable Long Mode + NX in EFER
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8) | (1 << 11)    ; LME | NXE
    wrmsr

    ; 10. Enable Paging + Write Protect
    mov eax, cr0
    or eax, (1 << 31) | (1 << 16)   ; PG | WP
    mov cr0, eax

    ; 11. Load 64-bit GDT (physical pointer)
    lgdt [gdt64.pointer]

    ; 12. Far jump to 64-bit trampoline
    jmp gdt64.code_segment:long_mode_trampoline

bits 64
long_mode_trampoline:
    ; Reset segment registers to null descriptor
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 13. Jump across the boundary into the HIGHER HALF!
    mov rax, higher_half_entry
    jmp rax

; ==============================================================================
; Higher-Half Kernel Code & Stack (Virtual memory >= 0xFFFFFFFF80000000)
; ==============================================================================
section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KiB 64-bit kernel stack
stack_top:

section .text
higher_half_entry:
    ; RIP is now 0xFFFFFFFF8010xxxx!

    ; Set up 64-bit higher-half stack pointer
    mov rsp, stack_top

    ; Zero-extend 32-bit Multiboot arguments into 64-bit ABI registers:
    ; RDI = 1st argument (magic)
    ; RSI = 2nd argument (multiboot_info physical address)
    mov edi, edi
    mov esi, esi

    ; Call 64-bit C kernel_main in higher half
    call kernel_main

    ; Halt the CPU if kernel_main ever returns
    cli
.halt:
    hlt
    jmp .halt

section .note.GNU-stack noalloc noexec nowrite progbits
