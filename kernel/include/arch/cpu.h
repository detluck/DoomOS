#pragma once

#include "types.h"

#define RFLAGS_INTERRUPT_ENABLE (1ULL << 9)

static inline u64 read_rflags(void)
{
    u64 flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
}
