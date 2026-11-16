#pragma once

#include "types.h"

#define RFLAGS_INTERRUPT_ENABLE (1ULL << 9)

/* Only the CPU observations used by the real dispatcher and PIC code are replaced. */
u64 read_rflags(void);
u64 read_cr2(void);
