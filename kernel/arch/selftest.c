#include "arch/selftest.h"
#include "arch/boot.h"
#include "arch/cpu.h"
#include "lib/assert.h"
#include "mm/vmm.h"
#include "vga.h"

#ifdef ARCH_SELFTEST

#define TEST_GP_SELECTOR 0x38
#define TEST_PF_ADDRESS 0x10000000ULL

extern void arch_test_ud(void) __noreturn;
extern void arch_test_gp(u64 selector) __noreturn;
extern void arch_test_pf(virt_addr_t address) __noreturn;
extern const u8 arch_test_ud_fault[], arch_test_gp_fault[], arch_test_pf_fault[];

#ifdef ARCH_TEST_GP
static void check_gp_selector(void)
{
    struct {
        u16 limit;
        u64 base;
    } __packed gdtr;
    __asm__ volatile("sgdt %0" : "=m"(gdtr) : : "memory");
    assert(TEST_GP_SELECTOR > gdtr.limit);
}
#endif

#if defined(ARCH_TEST_UD) || defined(ARCH_TEST_GP) || defined(ARCH_TEST_PF)
static void show_trigger(const char *name, const u8 *address)
{
    vga_write("ARCH_TEST ");
    vga_write(name);
    vga_write(" trigger RIP=0x");
    vga_write_hex((uintptr_t) address);
    vga_write("\n");
}
#endif

/* Assembly falls through here if an expected fatal instruction did not fault. */
void arch_test_unexpected_return(void) __noreturn;
void arch_test_unexpected_return(void)
{
    panic("ARCH_TEST FAIL: expected CPU exception did not occur");
}

void arch_selftest_run(void)
{
    assert(!(read_rflags() & RFLAGS_INTERRUPT_ENABLE));
#if defined(ARCH_TEST_SMOKE)
    vga_write("ARCH_TEST smoke PASS\n");
    cpu_halt_forever();
#elif defined(ARCH_TEST_UD)
    show_trigger("ud", arch_test_ud_fault);
    arch_test_ud();
#elif defined(ARCH_TEST_GP)
    check_gp_selector();
    show_trigger("gp", arch_test_gp_fault);
    arch_test_gp(TEST_GP_SELECTOR);
#elif defined(ARCH_TEST_PF)
    pml4_t *active_pml4 = (pml4_t *) PHYS_TO_VIRT_DIRECT(vmm_read_cr3());
    assert(!vmm_is_mapped(active_pml4, TEST_PF_ADDRESS));
    show_trigger("pf", arch_test_pf_fault);
    arch_test_pf(TEST_PF_ADDRESS);
#elif defined(ARCH_TEST_PANIC_REENTRY)
    panic_begin();
    assert(!(read_rflags() & RFLAGS_INTERRUPT_ENABLE));
    vga_write("ARCH_TEST panic-reentry: first entry, IF=0\n");
    panic("ARCH_TEST FAIL: second panic reached console output");
#else
#error "ARCH_SELFTEST requires a supported test selection"
#endif
}

#endif
