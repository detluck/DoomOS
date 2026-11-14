#pragma once

#include "tss.h"

#define GDT_KERNEL_CODE_SELECTOR 0x08
#define GDT_KERNEL_DATA_SELECTOR 0x10
#define GDT_USER_DATA_SELECTOR 0x1B
#define GDT_USER_CODE_SELECTOR 0x23
#define GDT_TSS_SELECTOR 0x28

/* Early boot only: long mode, VGA and stack ready; interrupts disabled. */
void gdt_init(void);

/* Requires the active kernel GDT, IF=0 and a prepared 104-byte TSS.
 * Installs its descriptor and loads TR once; base must be in the early mapping.
 */
void gdt_load_tss(const tss_t *state);
