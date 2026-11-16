#include "arch/irq.h"
#include "arch/cpu.h"
#include "arch/pic.h"
#include "lib/assert.h"

_Static_assert(PIC_SLAVE_VECTOR_BASE == PIC_MASTER_VECTOR_BASE + PIC_IRQ_COUNT / 2,
               "PIC vector ranges must be contiguous");

static irq_handler_t handlers[PIC_IRQ_COUNT];
static u16 unhandled_mask;

static void irq_check_disabled(void)
{
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("IRQ: interrupts must be disabled");
    }
}

bool irq_register_handler(u8 irq, irq_handler_t handler)
{
    irq_check_disabled();
    if (irq >= PIC_IRQ_COUNT || irq == PIC_CASCADE_IRQ || !handler || handlers[irq]) {
        return false;
    }
    if (!(pic_get_mask() & (1U << irq))) {
        return false;
    }
    handlers[irq] = handler;
    return true;
}

bool irq_dispatch(const interrupt_context_t *context)
{
    if (context->vector < PIC_MASTER_VECTOR_BASE ||
        context->vector >= PIC_MASTER_VECTOR_BASE + PIC_IRQ_COUNT) {
        return false;
    }
    irq_check_disabled();
    u8 irq = context->vector - PIC_MASTER_VECTOR_BASE;
    if (irq == PIC_CASCADE_IRQ) {
        panic("IRQ: unexpected cascade vector");
    }
    if (!pic_begin_irq(irq)) {
        return true;
    }
    irq_handler_t handler = handlers[irq];
    if (handler) {
        handler(context);
        irq_check_disabled();
    } else {
        if (!pic_mask_irq(irq)) {
            panic("IRQ: failed to mask unhandled interrupt");
        }
    }
    pic_end_irq(irq);
    if (!handler) {
        /* Console output could reenter an interrupted VGA operation. Defer it. */
        unhandled_mask |= 1U << irq;
    }
    return true;
}

u16 irq_get_unhandled_mask(void)
{
    irq_check_disabled();
    return unhandled_mask;
}
