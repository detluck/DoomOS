#include "vga.h"
#include "gdt.h"
#include "types.h"
#include "arch/io.h"
#include "mm/multiboot2.h"
#include "mm/pmm.h"
#include "lib/assert.h"
#include "lib/kprintf.h"

void kernel_main(u64 magic, u64 mb2_info_adr)
{
    vga_init();

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_CYAN);
    vga_write("DoomOS 64-bit kernel initialized.\n");
    vga_write("VGA text driver online: 80x25, hardware cursor, scrolling.\n\n");

    gdt_init();

    if (!multiboot_init(magic, mb2_info_adr)) {
        panic("Failed to initialize Multiboot2: cannot detect physical memory!");
    }
    print_multiboot_mmap();

    pmm_init();
    pmm_test();

    // kprintf examples
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("\nkprintf: %s %c %d %u %x %%\n", "hello", 'A', -42, 42u, 0xbeefu);
    kprintf("kprintf: free frames %lu / %lu\n", pmm_get_free_frames(), pmm_get_total_frames());
    kprintf("kprintf: kernel_main at %p\n", (void *) kernel_main);

    vga_set_color(VGA_COLOR_BLUE, VGA_COLOR_GREEN);

    vga_write("\nShutting down soons...\n");

    // temporary wait
    for (volatile uint64_t i = 0; i < 2000000000ULL; i++)
        ;

    vga_clear();

    vga_set_color(VGA_COLOR_BLUE, VGA_COLOR_DARK_GRAY);
    vga_write("Powering off.\n");
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_MAGENTA);
    vga_write("bye");

    for (volatile uint64_t i = 0; i < 2000000000ULL; i++)
        ;

    /* QEMU ACPI shutdown: write 0x2000 to port 0x604 (pm1a_cnt) */
    outw(0x604, 0x2000);

    while (1) {
        __asm__ volatile("hlt");
    }
}
