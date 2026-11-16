#pragma once

#include "types.h"
#include "arch/interrupt_context.h"

/* Borrowed, read-only context. Do not retain it, enable IRQs, wait, or send PIC EOIs. */
typedef void (*irq_handler_t)(const interrupt_context_t *context);

/* Single CPU, IF=0, PIC initialized, device IRQ masked. Does not unmask it.
 * Invalid IRQs, IRQ2, NULL, occupied slots and unmasked lines return false.
 * A handler function may serve multiple IRQs, but each IRQ has only one handler.
 */
bool irq_register_handler(u8 irq, irq_handler_t handler);

/* Architecture entry only. False means outside the PIC vector range, with no I/O.
 * PIC entries require IF=0; true includes fully handled spurious/unhandled IRQs.
 */
bool irq_dispatch(const interrupt_context_t *context);

/* IF=0. Latched unhandled device IRQ bits since boot, not the PIC mask. Does not clear. */
u16 irq_get_unhandled_mask(void);
