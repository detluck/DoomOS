#pragma once

#include "types.h"

/* Test-only replacement, selected before kernel/include for host_test_irq. */
u8 inb(u16 port);
void outb(u16 port, u8 value);

static inline void io_wait(void)
{
    outb(0x80, 0);
}
