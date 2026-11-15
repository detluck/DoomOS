#pragma once

#include "types.h"
#include "defs.h"

#define RFLAGS_INTERRUPT_ENABLE (1ULL << 9)

static __always_inline u64 read_cr2(void)
{
    u64 address;
    __asm__ volatile("mov %%cr2, %0" : "=r"(address) : : "memory");
    return address;
}

static inline void cpu_disable_interrupts(void)
{
    __asm__ volatile("cli" : : : "memory");
}

static inline __noreturn void cpu_halt_forever(void)
{
    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}

static inline u64 read_rflags(void)
{
    u64 flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
}
