#include "arch/interrupt_context.h"
#include "defs.h"
#include "lib/assert.h"
#include "lib/kprintf.h"

/* Called only by the assembly entry, with all GPRs saved and live DF cleared. */
void interrupt_dispatch(const interrupt_context_t *context, uintptr_t entry_rsp)
{
    if ((entry_rsp & 15) != 8 || entry_rsp >= (uintptr_t) context) {
        panic("ISR: invalid C entry stack");
    }
    kprintf("\nUnhandled interrupt: vector=%lu error=0x%lx\n", context->vector,
            context->error_code);
    panic("ISR: unhandled interrupt");
}
