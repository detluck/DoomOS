#include "tss.h"
#include "defs.h"
#include "arch/boot.h"
#include "arch/cpu.h"
#include "gdt.h"
#include "lib/assert.h"
#include "lib/string.h"
#include "types.h"

#define LOG_SCOPE "TSS"
#include "lib/log.h"

#define TSS_STACK_SIZE (16 * 1024)
#define TSS_ALIGNMENT 16
_Static_assert(TSS_ALIGNMENT > 0 && (TSS_ALIGNMENT & (TSS_ALIGNMENT - 1)) == 0,
               "TSS alignment must be a nonzero power of two");
_Static_assert(TSS_STACK_SIZE % TSS_ALIGNMENT == 0, "TSS stack size must preserve alignment");

extern u8 _kernel_start[];
extern u8 _kernel_end[];

static tss_t tss __aligned(TSS_ALIGNMENT);
static u8 rsp0_stack[TSS_STACK_SIZE] __aligned(TSS_ALIGNMENT);
static u8 ist1_stack[TSS_STACK_SIZE] __aligned(TSS_ALIGNMENT);
static bool initialized;

static void tss_check_region(uintptr_t base, size_t size)
{
    if (base < EARLY_MAPPING_START || base >= EARLY_MAPPING_END ||
        size > EARLY_MAPPING_END - base) {
        panic("TSS: required memory outside early identity mapping");
    }

    uintptr_t kernel_start = (uintptr_t) _kernel_start;
    uintptr_t kernel_end = (uintptr_t) _kernel_end;
    if (base < kernel_start || base >= kernel_end || size > kernel_end - base) {
        panic("TSS: required memory outside reserved kernel");
    }
    if (!IS_ALIGNED(base, TSS_ALIGNMENT)) {
        panic("TSS: required memory is not aligned");
    }
}

static void tss_check_storage(void)
{
    tss_check_region((uintptr_t) &tss, sizeof(tss));
    tss_check_region((uintptr_t) rsp0_stack, sizeof(rsp0_stack));
    tss_check_region((uintptr_t) ist1_stack, sizeof(ist1_stack));
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
    gdt_load_tss(&tss);
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("TSS: interrupts enabled after initialization");
    }
    initialized = true;
    pr_ok("TSS initialized and verified.");
}
