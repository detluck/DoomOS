#pragma once

#include "types.h"

/* Stack at isr_common after saving GPRs. All slots are eight bytes.
 * In 64-bit mode the CPU saves SS:RSP even for same-CPL, IST=0 entries.
 * Stubs supply only a missing error code and the vector. No packed layout needed.
 */
typedef struct {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rdi, rsi, rbp, rdx, rcx, rbx, rax;
    u64 vector;
    u64 error_code;
    u64 rip, cs, rflags, rsp, ss;
} interrupt_context_t;

#define INTERRUPT_OFFSET(field, offset) \
    _Static_assert(offsetof(interrupt_context_t, field) == (offset), "ISR offset: " #field)
INTERRUPT_OFFSET(r15, 0);
INTERRUPT_OFFSET(r14, 8);
INTERRUPT_OFFSET(r13, 16);
INTERRUPT_OFFSET(r12, 24);
INTERRUPT_OFFSET(r11, 32);
INTERRUPT_OFFSET(r10, 40);
INTERRUPT_OFFSET(r9, 48);
INTERRUPT_OFFSET(r8, 56);
INTERRUPT_OFFSET(rdi, 64);
INTERRUPT_OFFSET(rsi, 72);
INTERRUPT_OFFSET(rbp, 80);
INTERRUPT_OFFSET(rdx, 88);
INTERRUPT_OFFSET(rcx, 96);
INTERRUPT_OFFSET(rbx, 104);
INTERRUPT_OFFSET(rax, 112);
INTERRUPT_OFFSET(vector, 120);
INTERRUPT_OFFSET(error_code, 128);
INTERRUPT_OFFSET(rip, 136);
INTERRUPT_OFFSET(cs, 144);
INTERRUPT_OFFSET(rflags, 152);
INTERRUPT_OFFSET(rsp, 160);
INTERRUPT_OFFSET(ss, 168);
#undef INTERRUPT_OFFSET
_Static_assert(sizeof(interrupt_context_t) == 176, "ISR context must be 176 bytes");
