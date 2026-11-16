bits 64
default rel

extern interrupt_dispatch
global isr_stub_table
global isr_code_start, isr_code_end

section .text
isr_code_start:

; Intel SDM vol. 3A: #DF, #TS, #NP, #SS, #GP, #PF, #AC, #CP.
; AMD APM vol. 2 adds #VC (29) and #SX (30). These are CPU exceptions,
; never invoke an error-code vector using software INT.
%assign vector 0
%rep 256
isr_stub_%+vector:
%if vector != 8 && vector != 10 && vector != 11 && vector != 12 && vector != 13 \
    && vector != 14 && vector != 17 && vector != 21 && vector != 29 && vector != 30
    push qword 0
%endif
    push qword vector
    jmp isr_common
%assign vector vector + 1
%endrep

isr_common:
    ; Keep this order in sync with arch/interrupt_context.h.
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld
    mov rbx, rsp                 ; callee-saved anchor, original RBX is in context
    mov rdi, rbx
    and rsp, -16                 ; SysV: aligned before CALL
    call .dispatch_entry
    mov rsp, rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16                  ; vector and normalized error code only
    iretq

.dispatch_entry:
    mov rsi, rsp                 ; observe actual C-entry RSP before its prologue
    jmp interrupt_dispatch      ; tail call preserves the observed entry stack
isr_code_end:

section .rodata
align 8
isr_stub_table:
%assign vector 0
%rep 256
    dq isr_stub_%+vector
%assign vector vector + 1
%endrep

section .note.GNU-stack noalloc noexec nowrite progbits
