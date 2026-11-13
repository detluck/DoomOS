#include "mm/vmm.h"
#include "vga.h"
#include "gdt.h"
#include "tss.h"
#include "types.h"
#include "arch/io.h"
#include "mm/multiboot2.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "lib/assert.h"
#include "lib/kprintf.h"

extern void run_heap_tests(void);
extern void run_pmm_tests(void);

static u8 ps2_read(void)
{
    while (!(inb(0x64) & 0x01))
        ;
    return inb(0x60);
}

static void print_scancode_set(void)
{
    outb(0x60, 0xF0);
    ps2_read(); // ACK
    outb(0x60, 0x00);
    ps2_read(); // ACK
    kprintf("scancode set reply: 0x%x\n", ps2_read());
}

void kernel_main(u64 magic, u64 mb2_info_adr)
{
    vga_init();

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_CYAN);
    vga_write("DoomOS 64-bit kernel initialized.\n");
    vga_write("VGA text driver online: 80x25, hardware cursor, scrolling.\n\n");

    gdt_init();
    tss_init();

    if (!multiboot_init(magic, mb2_info_adr)) {
        panic("Failed to initialize Multiboot2: cannot detect physical memory!");
    }
    print_multiboot_mmap();

    pmm_init();
    pmm_test();
    vmm_init();
    vmm_test();
    kmalloc_init();

// internal tests
#ifdef RUN_TESTS
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write("\n--- RUNNING IN-KERNEL TESTS ---\n");

    run_pmm_tests();
    run_heap_tests();

    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_write("--- ALL TESTS PASSED! ---\n");

    /* Automatically shut down QEMU when tests succeed */
    outw(0xF4, 0x10);

    while (1)
        __asm__ volatile("hlt");
#endif

    // kprintf examples
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("\nkprintf: %s %c %d %u %x %%\n", "hello", 'A', -42, 42u, 0xbeefu);
    kprintf("kprintf: free frames %lu / %lu\n", pmm_get_free_frames(), pmm_get_total_frames());
    kprintf("kprintf: kernel_main at %p\n", (void *) kernel_main);
    print_scancode_set(); // returned 0x41 - set 2

    /* QEMU CI smoke test: signal a successful boot via isa-debug-exit.
       QEMU exits with (0x10 << 1) | 1 = 33. Without the device this is a no-op. */
    outl(0xF4, 0x10);

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
