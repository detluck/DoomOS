global start
extern kernel_main

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 16384          ; 16 KiB early stack
stack_top:

section .text
bits 32
start:
    ; Set up initial 32-bit stack
    mov esp, stack_top

    ; Save Multiboot bootloader arguments in ABI registers
    mov edi, eax        ; 1st argument: Multiboot magic
    mov esi, ebx        ; 2nd argument: Multiboot info physical address

    ; 1. Set up page table hierarchy (identity-map first 2 MiB)
    ; Link P4 table to P3 table
    mov eax, p3_table
    or eax, 0b11        ; Present + Writable
    mov [p4_table], eax

    ; Link P3 table to P2 table
    mov eax, p2_table
    or eax, 0b11        ; Present + Writable
    mov [p3_table], eax

    ; Map first 2 MiB in P2 table using a huge page (bit 7 = 1)
    mov eax, 0x0
    or eax, 0b10000011  ; Present + Writable + Huge Page (2 MiB)
    mov [p2_table], eax

    ; 2. Load CR3 with address of P4 table
    mov eax, p4_table
    mov cr3, eax

    ; 3. Enable PAE (Physical Address Extension) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 4. Set Long Mode Enable (LME) bit in EFER MSR (0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 5. Enable Paging and Protected Mode in CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 6. Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; 7. Far jump to 64-bit code segment
    jmp gdt64.code_segment:long_mode_start

section .text
bits 64
long_mode_start:
    ; Reset segment registers to null descriptor in 64-bit mode
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up 64-bit stack pointer
    mov rsp, stack_top

    ; Zero-extend 32-bit Multiboot arguments into 64-bit ABI registers:
    ; RDI = 1st argument (magic)
    ; RSI = 2nd argument (multiboot_info physical address)
    mov edi, edi
    mov esi, esi

    ; Call 64-bit C kernel_main(magic, mb_info_addr)
    call kernel_main

    ; Halt the CPU if kernel_main ever returns
    cli
.halt:
    hlt
    jmp .halt

section .rodata
align 8
gdt64:
    dq 0                         ; Null descriptor
.code_segment equ $ - gdt64
    ; Executable, Readable, Code/Data, Present, Long Mode (64-bit)
    dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
