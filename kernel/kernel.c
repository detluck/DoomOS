#include "include/vga.h"
#include <stdint.h>

static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

void kernel_main(void)
{
    vga_init();

    vga_set_color(14, 3);
    vga_write("DoomOS 64-bit kernel initialized.\n");
    vga_write("VGA text driver online: 80x25, hardware cursor, scrolling.\n\n");

    vga_set_color(1, 2);

    vga_write("\nShutting down soons...\n");

    // temporary wait
    for (volatile uint64_t i = 0; i < 2000000000ULL; i++)
        ;

    vga_clear();

    vga_set_color(1, 8);
    vga_write("Powering off.\n");
    vga_set_color(3, 5);
    vga_write("bye");

    for (volatile uint64_t i = 0; i < 2000000000ULL; i++)
        ;

    /* QEMU ACPI shutdown: write 0x2000 to port 0x604 (pm1a_cnt) */
    outw(0x604, 0x2000);

    while (1) {
        __asm__ volatile("hlt");
    }
}