#include "gdt.h"
#include "defs.h"
#include "lib/assert.h"
#include "types.h"

#define LOG_SCOPE "GDT"
#include "lib/log.h"

#define GDT_ENTRY_COUNT 5
#define GDT_SELECTOR_INDEX(selector) ((selector) >> 3)
#define GDT_EARLY_MAPPING_END 0x200000ULL
#define RFLAGS_INTERRUPT_ENABLE (1ULL << 9)

#define GDT_ACCESS_ACCESSED (1U << 0)
#define GDT_ACCESS_READ_WRITE (1U << 1)
#define GDT_ACCESS_EXECUTABLE (1U << 3)
#define GDT_ACCESS_CODE_DATA (1U << 4)
#define GDT_ACCESS_RING3 (3U << 5)
#define GDT_ACCESS_PRESENT (1U << 7)
#define GDT_ACCESS_DATA \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_CODE_DATA | GDT_ACCESS_READ_WRITE | GDT_ACCESS_ACCESSED)
#define GDT_ACCESS_CODE (GDT_ACCESS_DATA | GDT_ACCESS_EXECUTABLE)

#define GDT_FLAG_LONG_MODE (1U << 1)
#define GDT_FLAG_DEFAULT_SIZE (1U << 2)
#define GDT_FLAG_GRANULARITY (1U << 3)
#define GDT_FLAGS_CODE (GDT_FLAG_GRANULARITY | GDT_FLAG_LONG_MODE)
#define GDT_FLAGS_DATA (GDT_FLAG_GRANULARITY | GDT_FLAG_DEFAULT_SIZE)

/* Base = 0, limit = 0xFFFFF; access byte and upper flag nibble are explicit. */
#define GDT_DESCRIPTOR(access, flags) \
    (0xFFFFULL | ((u64) (access) << 40) | (0xFULL << 48) | ((u64) (flags) << 52))

typedef u64 gdt_descriptor_t;

typedef struct {
    u16 limit;
    u64 base;
} __packed gdt_pointer_t;

typedef struct {
    u16 cs;
    u16 ds;
    u16 es;
    u16 ss;
    u16 fs;
    u16 gs;
} segment_selectors_t;

static gdt_descriptor_t gdt[GDT_ENTRY_COUNT] __aligned(8) = {
    [0] = 0,
    [GDT_SELECTOR_INDEX(GDT_KERNEL_CODE_SELECTOR)] = GDT_DESCRIPTOR(GDT_ACCESS_CODE,
                                                                    GDT_FLAGS_CODE),
    [GDT_SELECTOR_INDEX(GDT_KERNEL_DATA_SELECTOR)] = GDT_DESCRIPTOR(GDT_ACCESS_DATA,
                                                                    GDT_FLAGS_DATA),
    [GDT_SELECTOR_INDEX(GDT_USER_DATA_SELECTOR)] = GDT_DESCRIPTOR(GDT_ACCESS_DATA |
                                                                      GDT_ACCESS_RING3,
                                                                  GDT_FLAGS_DATA),
    [GDT_SELECTOR_INDEX(GDT_USER_CODE_SELECTOR)] = GDT_DESCRIPTOR(GDT_ACCESS_CODE |
                                                                      GDT_ACCESS_RING3,
                                                                  GDT_FLAGS_CODE),
};

static const gdt_pointer_t gdt_pointer = {
    .limit = sizeof(gdt) - 1,
    .base = (uintptr_t) gdt,
};

_Static_assert(sizeof(gdt_descriptor_t) == 8, "GDT descriptor must be 8 bytes");
_Static_assert(sizeof(gdt_pointer_t) == 10, "GDTR operand must be 10 bytes");
_Static_assert(offsetof(gdt_pointer_t, base) == 2, "GDTR base must start at offset 2");
_Static_assert(sizeof(gdt) == 40, "GDT must contain five descriptors");
_Static_assert(GDT_KERNEL_CODE_SELECTOR == (1 << 3), "Kernel code selector layout");
_Static_assert(GDT_KERNEL_DATA_SELECTOR == (2 << 3), "Kernel data selector layout");
_Static_assert(GDT_USER_DATA_SELECTOR == ((3 << 3) | 3), "User data selector layout");
_Static_assert(GDT_USER_CODE_SELECTOR == ((4 << 3) | 3), "User code selector layout");

/* Internal assembly entry; upper bits of narrow arguments are unspecified. */
void gdt_load(const gdt_pointer_t *pointer, u16 code_selector, u16 data_selector);

static u64 read_rflags(void)
{
    u64 flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
}

static gdt_pointer_t read_gdtr(void)
{
    gdt_pointer_t pointer;
    __asm__ volatile("sgdt %0" : "=m"(pointer) : : "memory");
    return pointer;
}

static segment_selectors_t read_segment_selectors(void)
{
    segment_selectors_t selectors;
    __asm__ volatile("mov %%cs, %0\n\t"
                     "mov %%ds, %1\n\t"
                     "mov %%es, %2\n\t"
                     "mov %%ss, %3\n\t"
                     "mov %%fs, %4\n\t"
                     "mov %%gs, %5"
                     : "=r"(selectors.cs), "=r"(selectors.ds), "=r"(selectors.es),
                       "=r"(selectors.ss), "=r"(selectors.fs), "=r"(selectors.gs));
    return selectors;
}

static void gdt_check_mapping(uintptr_t base, size_t size)
{
    if (base >= GDT_EARLY_MAPPING_END || size > GDT_EARLY_MAPPING_END - base) {
        panic("GDT: required memory outside early identity mapping");
    }
}

static void gdt_check_descriptors(void)
{
    /* Independent reference encodings: access 9B/93/F3/FB, flags A/C/C/A.
     * Accessed is preset; user entries reserve DPL=3 without entering ring 3.
     * Volatile reads check actual table memory before privileged loads.
     */
    const volatile gdt_descriptor_t *entries = gdt;
    if (entries[0] != 0 || entries[1] != 0x00AF9B000000FFFFULL ||
        entries[2] != 0x00CF93000000FFFFULL || entries[3] != 0x00CFF3000000FFFFULL ||
        entries[4] != 0x00AFFB000000FFFFULL) {
        panic("GDT: invalid segment descriptor encoding");
    }
}

static void gdt_check_cpu_state(void)
{
    gdt_pointer_t loaded_pointer = read_gdtr();
    segment_selectors_t selectors = read_segment_selectors();
    if (loaded_pointer.base != (uintptr_t) gdt) {
        panic("GDT: unexpected GDTR base");
    }
    if (loaded_pointer.limit != sizeof(gdt) - 1) {
        panic("GDT: unexpected GDTR limit");
    }

    if (selectors.cs != GDT_KERNEL_CODE_SELECTOR) {
        panic("GDT: unexpected kernel code selector");
    }
    if (selectors.ds != GDT_KERNEL_DATA_SELECTOR) {
        panic("GDT: unexpected DS selector");
    }
    if (selectors.es != GDT_KERNEL_DATA_SELECTOR) {
        panic("GDT: unexpected ES selector");
    }
    if (selectors.ss != GDT_KERNEL_DATA_SELECTOR) {
        panic("GDT: unexpected SS selector");
    }
    if (selectors.fs != 0) {
        panic("GDT: unexpected FS selector");
    }
    if (selectors.gs != 0) {
        panic("GDT: unexpected GS selector");
    }
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("GDT: interrupts enabled after initialization");
    }
}

void gdt_init(void)
{
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("GDT: interrupts must be disabled before initialization");
    }
    gdt_check_mapping((uintptr_t) gdt, sizeof(gdt));
    gdt_check_mapping((uintptr_t) &gdt_pointer, sizeof(gdt_pointer));
    gdt_check_descriptors();
    gdt_load(&gdt_pointer, GDT_KERNEL_CODE_SELECTOR, GDT_KERNEL_DATA_SELECTOR);
    gdt_check_cpu_state();
    pr_ok("GDT initialized and verified.");
}
