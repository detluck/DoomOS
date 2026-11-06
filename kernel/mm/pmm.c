#include "mm/pmm.h"
#include "defs.h"
#include "lib/assert.h"
#include "lib/string.h"
#include "mm/multiboot2.h"
#include "mm/vmm.h"
#include "types.h"

#define LOG_SCOPE "PMM"
#include "lib/log.h"

extern u8 _kernel_phys_end[];
extern u8 _kernel_phys_start[];
extern u8 _kernel_end[];
extern u8 _kernel_start[];

static u64 *bitmap = NULL;
static phys_addr_t bitmap_phys = 0;
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
            pfn_t start_pfn = PHYS_TO_PFN(ALIGN_UP(entry->addr, PAGE_SIZE));
            pfn_t end_pfn = PHYS_TO_PFN(ALIGN_DOWN(entry->addr + entry->len, PAGE_SIZE));

            for (pfn_t pfn = start_pfn; pfn < end_pfn; pfn++) {
                if (bitmap_test(pfn)) {
                    bitmap_clear(pfn);
                    total_free_frames++;
                }
            }
        }
        ptr += mmap->entry_size;
    }
}

static void pmm_reserve_range(phys_addr_t start, u64 size)
{
    if (size == 0) {
        return;
    }
    pfn_t start_pfn = PHYS_TO_PFN(ALIGN_DOWN(start, PAGE_SIZE));
    pfn_t end_pfn = PHYS_TO_PFN(ALIGN_UP(start + size, PAGE_SIZE));

    for (pfn_t pfn = start_pfn; pfn < end_pfn; pfn++) {
        if (pfn < total_frames) {
            if (!bitmap_test(pfn)) {
                bitmap_set(pfn);
                total_free_frames--;
            }
        }
    }
}

static phys_addr_t pmm_find_bitmap_location(phys_addr_t min, u64 size)
{
    const struct multiboot_tag_mmap *mmap = get_multiboot_mmap();
    const u8 *end = (const u8 *) mmap + mmap->size;

    for (const u8 *p = (const u8 *) mmap->entries; p < end; p += mmap->entry_size) {
        const struct multiboot_mmap_entry *e = (const struct multiboot_mmap_entry *) p;
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }
        phys_addr_t start = ALIGN_UP(e->addr > min ? e->addr : min, PAGE_SIZE);
        phys_addr_t stop = e->addr + e->len;
        if (stop > BOOT_DIRECT_MAP_SIZE) {
            stop = BOOT_DIRECT_MAP_SIZE;
        }
        if (start < stop && stop - start >= size) {
            return start;
        }
    }
    return 0;
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
    // total_frames from the multiboot2 memory map
    pmm_calculate_start_frames();

    // 2. Find where the multiboot2 info lives, so the bitmap can't land on it
    phys_addr_t mb2_phys = 0;
    u64 mb2_size = 0;
    const bool have_mb2 = multiboot_get_bounds(&mb2_phys, &mb2_size);

    // place the bitmap after the kernel and after the MB2 info 8-byte aligned,
    // and access it through the direct map
    phys_addr_t bm_start = (phys_addr_t) _kernel_phys_end;
    if (have_mb2 && mb2_phys + mb2_size > bm_start) {
        bm_start = mb2_phys + mb2_size;
    }
    const u64 bitmap_size_words = (total_frames + 63) / 64;
    const u64 bitmap_size_bytes = bitmap_size_words * sizeof(u64);
    bitmap_phys = pmm_find_bitmap_location(bm_start, bitmap_size_bytes);
    if (bitmap_phys == 0) {
        panic("PMM: no usable memory for the frame bitmap");
    }
    bitmap = (u64 *) PHYS_TO_VIRT_DIRECT(bitmap_phys);
    // mark all frames as used
    memset(bitmap, 0xFF, bitmap_size_bytes);

    // free every frame the memory map reports as available
    total_free_frames = 0;
    pmm_set_start_free_frames();

    // re-reserve everything that must not be handed out
    pmm_reserve_range(0x0, 0x100000);                                       // BIOS, VGA, etc.
    pmm_reserve_range(0x100000, (phys_addr_t) _kernel_phys_end - 0x100000); // kernel image
    pmm_reserve_range(bitmap_phys, bitmap_size_bytes);                      // the bitmap itself
    if (have_mb2) {
        pmm_reserve_range(mb2_phys, mb2_size); // MB2 info
    }
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
                    pr_err("PFN exceeds total frame count");
                    break;
                }
                if (!bitmap_test(pfn)) {
                    bitmap_set(pfn);
                    total_free_frames--;
                    return PFN_TO_PHYS(pfn);
                }
            }
        }
    }

    return 0;
}

void pmm_free_frame(phys_addr_t phys_addr)
{
    if (phys_addr & (PAGE_SIZE - 1)) {
        pr_err("Trying to free unaligned frame");
        return;
    }
    const pfn_t pfn = PHYS_TO_PFN(phys_addr);
    if (pfn >= total_frames) {
        pr_err("PFN exceeds total frame count");
        return;
    }
    if (!bitmap_test(pfn)) {
        pr_err("Trying to free already free frame");
        return;
    }
    bitmap_clear(pfn);
    total_free_frames++;
}

void pmm_test(void)
{

    const u64 free_start = pmm_get_free_frames();
    assert(free_start > 0);

    // 1. Allocate a single frame and check alignment
    phys_addr_t f1 = pmm_alloc_frame();
    assert(f1 != 0);
    assert((f1 & (PAGE_SIZE - 1)) == 0);
    assert(pmm_get_free_frames() == free_start - 1);

    // 2. Verify frame is NOT in reserved memory zones
    assert(f1 >= 0x100000);
    assert(f1 >= (phys_addr_t) _kernel_phys_end);
    const u64 bitmap_size_words = (total_frames + 63) / 64;
    const u64 bitmap_size_bytes = bitmap_size_words * sizeof(u64);
    assert(f1 < bitmap_phys || f1 >= bitmap_phys + bitmap_size_bytes);

    // 3. Allocate multiple consecutive frames and ensure uniqueness
    phys_addr_t f2 = pmm_alloc_frame();
    phys_addr_t f3 = pmm_alloc_frame();
    assert(f2 != 0 && f3 != 0);
    assert(f1 != f2 && f2 != f3 && f1 != f3);
    assert(pmm_get_free_frames() == free_start - 3);

    // 4. Free one frame and verify recycling
    pmm_free_frame(f2);
    assert(pmm_get_free_frames() == free_start - 2);

    phys_addr_t f4 = pmm_alloc_frame();
    assert(f4 == f2);
    assert(pmm_get_free_frames() == free_start - 3);

    // 5. Clean up all test allocations
    pmm_free_frame(f1);
    pmm_free_frame(f3);
    pmm_free_frame(f4);
    assert(pmm_get_free_frames() == free_start);

    pr_ok("PMM self-test passed successfully!");
}
