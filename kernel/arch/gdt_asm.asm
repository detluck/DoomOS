bits 64

section .text
global gdt_load

; SysV: RDI = GDTR pointer, SI = code selector, DX = data selector.
; Called in 64-bit mode with interrupts disabled.
gdt_load:
    ; Discard unspecified upper bits of the 16-bit selector arguments.
    movzx esi, si
    movzx edx, dx

    ; LGDT selects the table; segment registers must be reloaded separately.
    lgdt [rdi]

    ; Reload CS. Stack at RETFQ: RIP, CS, original C return address.
    ; The same-ring far return consumes our RIP/CS pair.
    push rsi
    lea rax, [rel .reload_segments]
    push rax
    retfq

.reload_segments:
    ; Reload data/stack selectors; leave FS/GS and interrupt state untouched.
    mov ds, dx
    mov es, dx
    mov ss, dx

    ; Return to C using the original return address.
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
