#include "mm/multiboot2.h"
#include "mm/vmm.h"
#define LOG_SCOPE "MULTIBOOT"
#include "lib/log.h"
#include "vga.h"

static phys_addr_t mb2_phys = 0;
static u64 mb2_size = 0;
static const struct multiboot_tag_mmap *mmap_tag = NULL;

bool multiboot_init(u64 magic, u64 info_adr)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        pr_err("Magic is not equal to multiboot2 specs");
        return false;
    }
    const struct multiboot_info *info =
        (const struct multiboot_info *) PHYS_TO_VIRT_DIRECT(info_adr);

    if (info_adr == 0 || (info_adr & 7)) {
        pr_err("Invalid multiboot info address");
        return false;
    }
    if (info->total_size < 16) {
        pr_err("Size field in info struct < 16");
        return false;
    }
    if (info->reserved != 0) {
        pr_err("Reserved field is not 0");
        return false;
    }

    mb2_phys = (phys_addr_t) info_adr;
    mb2_size = info->total_size;

    const u8 *base = (const u8 *) info;
    const u8 *end = (const u8 *) base + info->total_size;
    u8 *ptr = (u8 *) base + 8;

    while (ptr + sizeof(struct multiboot_tag) <= end) {
        const struct multiboot_tag *tag = (const struct multiboot_tag *) ptr;
        if (tag->size < 8 || ptr + tag->size > end) {
            pr_err("Corrupt tag with size 0");
            break;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap_tag = (const struct multiboot_tag_mmap *) tag;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }
        ptr += (tag->size + 7) & ~7;
    }
    return mmap_tag != NULL;
}

bool multiboot_get_bounds(phys_addr_t *phys_start, u64 *size)
{
    if (mb2_phys == 0 || mb2_size == 0) {
        return false;
    }
    if (phys_start) {
        *phys_start = mb2_phys;
    }
    if (size) {
        *size = mb2_size;
    }
    return true;
}

u64 multiboot_get_max_phys(void)
{
    u64 max = 0;
    if (!mmap_tag) {
        return 0;
    }
    const u8 *end = (const u8 *) mmap_tag + mmap_tag->size;
    for (const u8 *p = (const u8 *) mmap_tag->entries; p < end; p += mmap_tag->entry_size) {
        const struct multiboot_mmap_entry *e = (const struct multiboot_mmap_entry *) p;
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE || e->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE ||
            e->type == MULTIBOOT_MEMORY_NVS) {
            if (e->addr + e->len > max) {
                max = e->addr + e->len;
            }
        }
    }
    return max;
}

const struct multiboot_tag_mmap *get_multiboot_mmap(void)
{
    return mmap_tag;
}

void print_multiboot_mmap(void)
{
    if (mmap_tag == NULL) {
        pr_err("mmap_tag is NULL");
        return;
    }

    pr_info("--- Multiboot2 Memory Map ---");
    const u8 *base = (const u8 *) mmap_tag;
    const u8 *end = (const u8 *) base + mmap_tag->size;
    const u8 *ptr = (const u8 *) mmap_tag->entries;

    while (ptr < end) {
        const struct multiboot_mmap_entry *entry = (const struct multiboot_mmap_entry *) ptr;

        vga_write("  [");
        vga_write_hex(entry->addr);
        vga_write(" - ");
        vga_write_hex(entry->addr + entry->len);
        vga_write("] ");

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_write("AVAILABLE\n");
        } else if (entry->type == MULTIBOOT_MEMORY_RESERVED) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_write("RESERVED\n");
        } else if (entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
            vga_write("ACPI RECLAIMABLE\n");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);
            vga_write("OTHER\n");
        }
        vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);

        ptr += mmap_tag->entry_size;
    }
}
