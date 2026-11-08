#pragma once

#include "defs.h"
#include "types.h"

typedef struct {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    /* Array index 0 corresponds to hardware IST1. */
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __packed tss_t;

_Static_assert(sizeof(tss_t) == 104, "64-bit TSS must be 104 bytes");
_Static_assert(offsetof(tss_t, rsp0) == 4, "TSS RSP0 must start at offset 4");
_Static_assert(offsetof(tss_t, rsp1) == 12, "TSS RSP1 must start at offset 12");
_Static_assert(offsetof(tss_t, rsp2) == 20, "TSS RSP2 must start at offset 20");
_Static_assert(offsetof(tss_t, ist[0]) == 36, "TSS IST1 must start at offset 36");
_Static_assert(offsetof(tss_t, ist[6]) == 84, "TSS IST7 must start at offset 84");
_Static_assert(offsetof(tss_t, iomap_base) == 102, "TSS I/O map base must start at offset 102");

/* Early boot only: kernel GDT and VGA ready; interrupts disabled. Call once.
 * Prepares static RSP0/IST1 stacks and loads TR after checking their storage.
 * Does not switch away from the boot stack or install an interrupt gate.
 */
void tss_init(void);
