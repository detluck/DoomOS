#include "lib/assert.h"
#include "vga.h"
#include "arch/io.h"
#include "arch/cpu.h"

static volatile bool panic_active;

void panic_begin(void)
{
    cpu_disable_interrupts();
    if (panic_active) {
        panic_halt();
    }
    panic_active = true;

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write("\n================ KERNEL PANIC ================\n");
}

void panic_halt(void)
{
    cpu_disable_interrupts();
#ifdef RUN_TESTS
    outb(0xF4, 0x11); // exit code 35: a test failed
#endif
    cpu_halt_forever();
}

void panic(const char *message)
{
    panic_begin();
    vga_write(message);
    vga_write("\nSystem halted.\n");
    panic_halt();
}
