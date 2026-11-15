bits 64
default rel

extern arch_test_unexpected_return
global arch_test_ud, arch_test_gp, arch_test_pf
global arch_test_ud_fault, arch_test_gp_fault, arch_test_pf_fault

section .text

; Fatal test entries never return. RDI retains the trigger argument; RSP stays valid.
; Known GPR values make the saved context visible without an ISR return test.
%macro prepare_registers 0
    mov rax, 0x1111111111111111
    mov rbx, 0x2222222222222222
    mov rcx, 0x3333333333333333
    mov rdx, 0x4444444444444444
    mov rsi, 0x5555555555555555
    mov rbp, 0x7777777777777777
    mov r8,  0x8888888888888888
    mov r9,  0x9999999999999999
    mov r10, 0xAAAAAAAAAAAAAAAA
    mov r11, 0xBBBBBBBBBBBBBBBB
    mov r12, 0xCCCCCCCCCCCCCCCC
    mov r13, 0xDDDDDDDDDDDDDDDD
    mov r14, 0xEEEEEEEEEEEEEEEE
    mov r15, 0xFFFFFFFFFFFFFFFF
%endmacro

arch_test_ud:
    prepare_registers
    mov rdi, 0x6666666666666666
arch_test_ud_fault:
    ud2
    jmp arch_test_unexpected_return

arch_test_gp:
    prepare_registers
    mov ax, di
arch_test_gp_fault:
    mov ds, ax
    jmp arch_test_unexpected_return

arch_test_pf:
    prepare_registers
arch_test_pf_fault:
    mov rax, [rdi]
    jmp arch_test_unexpected_return

section .note.GNU-stack noalloc noexec nowrite progbits
