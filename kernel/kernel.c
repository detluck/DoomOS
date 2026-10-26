#include "vga.h"
#include "types.h"
#include "arch/io.h"
#include "mm/multiboot2.h"
#include "mm/pmm.h"
#include "lib/assert.h"

void kernel_main(u64 magic, u64 mb2_info_adr)
{
    vga_init();

    vga_set_color(14, 3);
    vga_write("DoomOS 64-bit kernel initialized.\n");
    vga_write("VGA text driver online: 80x25, hardware cursor, scrolling.\n\n");
    if (!multiboot_init(magic, mb2_info_adr)) {
        panic("Failed to initialize Multiboot2: cannot detect physical memory!");
    }
    print_multiboot_mmap();

    pmm_init();
    pmm_test();

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
