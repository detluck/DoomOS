#include "arch/interrupt_context.h"
#include "arch/cpu.h"
#include "arch/exceptions.h"
#include "defs.h"
#include "lib/assert.h"
#include "vga.h"

/* Called only by the assembly entry, with all GPRs saved and live DF cleared. */
void interrupt_dispatch(const interrupt_context_t *context, uintptr_t entry_rsp)
{
    /* Capture #PF's address before diagnostics can themselves fault. */
    u64 fault_address = context->vector == EXCEPTION_PAGE_FAULT ? read_cr2() : 0;
    if ((entry_rsp & 15) != 8 || entry_rsp >= (uintptr_t) context) {
        panic("ISR: invalid C entry stack");
    }
    if (context->vector < EXCEPTION_COUNT) {
        exception_panic(context, fault_address);
    }
    panic_begin();
    vga_write("Unhandled interrupt: vector=0x");
    vga_write_hex(context->vector);
    vga_write(" error=0x");
    vga_write_hex(context->error_code);
    vga_write("\nSystem halted.\n");
    panic_halt();
}
