#pragma once

#include "types.h"

#define PIC_MASTER_VECTOR_BASE 0x20
#define PIC_SLAVE_VECTOR_BASE 0x28
#define PIC_IRQ_COUNT 16
#define PIC_CASCADE_IRQ 2

/* Single CPU, IF=0 for every call. Initialize once before using the other functions. */
void pic_init(void);

/* IRQ numbers, not vectors. IRQ2 is managed internally as the slave cascade.
 * Invalid IRQs and IRQ2 return false without changing the masks.
 */
bool pic_mask_irq(u8 irq);
bool pic_unmask_irq(u8 irq);

/* Actual IMRs: master in bits 0-7, slave in bits 8-15; set means masked. */
u16 pic_get_mask(void);
