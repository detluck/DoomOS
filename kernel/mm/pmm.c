#include "mm/pmm.h"
#include "lib/assert.h"
#include "lib/string.h"
#include "mm/multiboot2.h"
#include "types.h"

#define LOG_SCOPE "PMM"
#include "lib/log.h"

extern u8 _kernel_end[];
extern u8 _kernel_start[];

static u64 *bitmap = NULL;
static u64 total_frames = 0;
static u64 total_free_frames = 0;

static void bitmap_set(pfn_t pfn)
{
    bitmap[pfn / 64] |= 1ULL << (pfn % 64);
}

static void bitmap_clear(pfn_t pfn)
{
    bitmap[pfn / 64] &= ~(1ULL << (pfn % 64));
}

static bool bitmap_test(pfn_t pfn)
{
    return (bitmap[pfn / 64] & (1ULL << (pfn % 64))) != 0;
}

static void pmm_calculate_start_frames(void)
{
    const struct multiboot_tag_mmap *mmap = get_multiboot_mmap();
    u64 max_adress = 0;

    const u8 *end = (const u8 *) mmap + mmap->size;
    const u8 *ptr = (const u8 *) mmap->entries;

    while (ptr < end) {

        const struct multiboot_mmap_entry *entry = (const struct multiboot_mmap_entry *) ptr;
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            u64 top_addr = entry->addr + entry->len;
            if (top_addr > max_adress) {
                max_adress = top_addr;
            }
        }
        ptr += mmap->entry_size;
    }
    total_frames = max_adress / PAGE_SIZE;
}

static void pmm_set_start_free_frames(void)
{
    const struct multiboot_tag_mmap *mmap = get_multiboot_mmap();

    const u8 *end = (const u8 *) mmap + mmap->size;
    const u8 *ptr = (const u8 *) mmap->entries;

    while (ptr < end) {

        const struct multiboot_mmap_entry *entry = (const struct multiboot_mmap_entry *) ptr;
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            pfn_t start_pfn = PHYS_TO_PFN(entry->addr);
            pfn_t end_pfn = PHYS_TO_PFN(entry->addr + entry->len);

            for (pfn_t pfn = start_pfn; pfn < end_pfn; pfn++) {
                bitmap_clear(pfn);
                total_free_frames++;
            }
        }
        ptr += mmap->entry_size;
    }
}

static void pmm_reserve_range(phys_addr_t start, u64 size)
{
    pfn_t start_pfn = PHYS_TO_PFN(start);
    pfn_t end_pfn = PHYS_TO_PFN(start + size + PAGE_SIZE - 1);

    for (pfn_t pfn = start_pfn; pfn < end_pfn; pfn++) {
        if (pfn < total_frames) {
            if (!bitmap_test(pfn)) {
                bitmap_set(pfn);
                total_free_frames--;
            }
        }
    }
}

u64 pmm_get_total_frames(void)
{
    return total_frames;
}
u64 pmm_get_free_frames(void)
{
    return total_free_frames;
}

void pmm_init(void)
{
    // calculate total frames given from multiboot2
    pmm_calculate_start_frames();

    bitmap = (u64 *) _kernel_end;

    // set all frames as used by default
    u64 bitmap_size_words = (total_frames + 63) / 64;
    u64 bitmap_size_bytes = bitmap_size_words * sizeof(u64);
    memset(bitmap, 0xFF, bitmap_size_bytes);

    // search free ram frames, set them to 0 and calculate total_free_frames
    total_free_frames = 0;
    pmm_set_start_free_frames();

    // reserve first MiB for BIOS, VGA, etc...
    pmm_reserve_range(0x0, 0x100000);
    // reserve kernel code
    pmm_reserve_range((u64) _kernel_start, (u64) _kernel_end - (u64) _kernel_start);
    // reserve bitmap itself
    pmm_reserve_range((u64) bitmap, bitmap_size_bytes);
    // reserve mmap struct
    const struct multiboot_tag_mmap *mmap = get_multiboot_mmap();
    pmm_reserve_range((u64) mmap, mmap->size);
}

phys_addr_t pmm_alloc_frame(void)
{
    if (total_free_frames == 0) {
        panic("OOM");
        return 0;
    }
    const u64 bitmap_size_words = (total_frames + 63) / 64;
    for (u64 i = 0; i < bitmap_size_words; i++) {
        if (bitmap[i] != ~0ULL) {
            for (u8 bit = 0; bit < 64; bit++) {
                pfn_t pfn = (i * 64) + bit;
                if (pfn >= total_frames) {
                    pr_err("Pfn is bigger then total frames number");
                    break;
                }
                if (!bitmap_test(pfn)) {
                    bitmap_set(pfn);
                    total_free_frames--;
                    pr_info("Allocated pfn");
                    return PFN_TO_PHYS(pfn);
                }
            }
        }
    }

    return 0;
}

void pmm_free_frame(phys_addr_t phys_addr)
{
    const pfn_t pfn = PHYS_TO_PFN(phys_addr);
    if (pfn > total_frames) {
        pr_err("Pfn is bigger then total frames number");
        return;
    }
    if (!bitmap_test(pfn)) {
        pr_err("Trying to free alredy free frame");
        return;
    }
    bitmap_clear(pfn);
    total_free_frames++;
}
