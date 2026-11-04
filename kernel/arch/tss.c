#include "tss.h"
#include "defs.h"
#include "gdt.h"
#include "lib/assert.h"
#include "lib/string.h"
#include "types.h"

#define LOG_SCOPE "TSS"
#include "lib/log.h"

#define TSS_STACK_SIZE (16 * 1024)
#define TSS_ALIGNMENT 16
#define TSS_EARLY_MAPPING_END 0x200000ULL
#define RFLAGS_INTERRUPT_ENABLE (1ULL << 9)

typedef struct {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    /* Array index 0 corresponds to hardware IST1. */
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __packed tss_t;

extern u8 _kernel_start[];
extern u8 _kernel_end[];

static tss_t tss __aligned(TSS_ALIGNMENT);
static u8 rsp0_stack[TSS_STACK_SIZE] __aligned(TSS_ALIGNMENT);
static u8 ist1_stack[TSS_STACK_SIZE] __aligned(TSS_ALIGNMENT);
static bool initialized;

_Static_assert(sizeof(tss_t) == 104, "64-bit TSS must be 104 bytes");
_Static_assert(offsetof(tss_t, rsp0) == 4, "TSS RSP0 must start at offset 4");
_Static_assert(offsetof(tss_t, rsp1) == 12, "TSS RSP1 must start at offset 12");
_Static_assert(offsetof(tss_t, rsp2) == 20, "TSS RSP2 must start at offset 20");
_Static_assert(offsetof(tss_t, ist[0]) == 36, "TSS IST1 must start at offset 36");
_Static_assert(offsetof(tss_t, ist[6]) == 84, "TSS IST7 must start at offset 84");
_Static_assert(offsetof(tss_t, iomap_base) == 102, "TSS I/O map base must start at offset 102");

static u64 read_rflags(void)
{
    u64 flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
}

static void tss_check_region(uintptr_t base, size_t size)
{
    if (base >= TSS_EARLY_MAPPING_END || size > TSS_EARLY_MAPPING_END - base) {
        panic("TSS: required memory outside early identity mapping");
    }

    uintptr_t kernel_start = (uintptr_t) _kernel_start;
    uintptr_t kernel_end = (uintptr_t) _kernel_end;
    if (base < kernel_start || base >= kernel_end || size > kernel_end - base) {
        panic("TSS: required memory outside reserved kernel");
    }
    if (base % TSS_ALIGNMENT != 0) {
        panic("TSS: required memory is not aligned");
    }
}

static void tss_check_storage(void)
{
    tss_check_region((uintptr_t) &tss, sizeof(tss));
    tss_check_region((uintptr_t) rsp0_stack, sizeof(rsp0_stack));
    tss_check_region((uintptr_t) ist1_stack, sizeof(ist1_stack));

    uintptr_t rsp0_bottom = (uintptr_t) rsp0_stack;
    uintptr_t rsp0_top = (uintptr_t) (rsp0_stack + sizeof(rsp0_stack));
    uintptr_t ist1_bottom = (uintptr_t) ist1_stack;
    uintptr_t ist1_top = (uintptr_t) (ist1_stack + sizeof(ist1_stack));
    if (rsp0_top > ist1_bottom && ist1_top > rsp0_bottom) {
        panic("TSS: RSP0 and IST1 stack ranges overlap");
    }
}

static void tss_check_contents(void)
{
    const volatile tss_t *state = &tss;
    if (state->rsp0 != (uintptr_t) (rsp0_stack + sizeof(rsp0_stack)) ||
        state->ist[0] != (uintptr_t) (ist1_stack + sizeof(ist1_stack))) {
        panic("TSS: unexpected stack pointers");
    }
    if (state->rsp0 % TSS_ALIGNMENT != 0 || state->ist[0] % TSS_ALIGNMENT != 0) {
        panic("TSS: stack pointers are not aligned");
    }
    if (state->reserved0 != 0 || state->reserved1 != 0 || state->reserved2 != 0 ||
        state->reserved3 != 0) {
        panic("TSS: reserved fields must be zero");
    }
    if (state->rsp1 != 0 || state->rsp2 != 0) {
        panic("TSS: unused privilege stacks must be zero");
    }
    for (size_t i = 1; i < 7; i++) {
        if (state->ist[i] != 0) {
            panic("TSS: unused IST pointers must be zero");
        }
    }
    if (state->iomap_base != 104) {
        panic("TSS: unexpected I/O bitmap offset");
    }
}

void tss_init(void)
{
    if (initialized) {
        panic("TSS: already initialized");
    }
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("TSS: interrupts must be disabled before initialization");
    }

    tss_check_storage();
    memset(&tss, 0, sizeof(tss));
    tss.rsp0 = (uintptr_t) (rsp0_stack + sizeof(rsp0_stack));
    tss.ist[0] = (uintptr_t) (ist1_stack + sizeof(ist1_stack));
    tss.iomap_base = sizeof(tss);
    tss_check_contents();
    gdt_load_tss((uintptr_t) &tss, sizeof(tss) - 1);
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("TSS: interrupts enabled after initialization");
    }
    initialized = true;
    pr_ok("TSS initialized and verified.");
}
