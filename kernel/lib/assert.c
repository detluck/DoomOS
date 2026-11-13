#include "lib/assert.h"
#include "vga.h"
#include "arch/io.h"

void panic(const char *message)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write("\n================ KERNEL PANIC ================\n");
    vga_write(message);
    vga_write("\nSystem halted.\n");

#ifdef RUN_TESTS

    outb(0xF4, 0x11); // exit code 35: a test failed
#endif

    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
