#include "arch/selftest.h"
#include "arch/boot.h"
#include "arch/cpu.h"
#include "arch/pic.h"
#include "idt.h"
#include "lib/assert.h"
#include "mm/vmm.h"
#include "vga.h"

#ifdef ARCH_SELFTEST

#define TEST_GP_SELECTOR 0x38
#define TEST_PF_ADDRESS 0x10000000ULL
#define TEST_DF_STACK_POINTER (TEST_PF_ADDRESS + PAGE_SIZE / 2)

extern void arch_test_ud(void) __noreturn;
extern void arch_test_gp(u64 selector) __noreturn;
extern void arch_test_pf(virt_addr_t address) __noreturn;
extern void arch_test_df_stack(virt_addr_t stack_pointer) __noreturn;
extern const u8 arch_test_ud_fault[], arch_test_gp_fault[], arch_test_pf_fault[];

#if defined(ARCH_TEST_GP) || defined(ARCH_TEST_DF)
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

#if defined(ARCH_TEST_PF) || defined(ARCH_TEST_DF_STACK)
static void check_unmapped_test_page(void)
{
    pml4_t *active_pml4 = (pml4_t *) PHYS_TO_VIRT_DIRECT(vmm_read_cr3());
    assert(IS_ALIGNED(TEST_PF_ADDRESS, PAGE_SIZE));
    assert(!vmm_is_mapped(active_pml4, TEST_PF_ADDRESS));
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

#if defined(ARCH_TEST_PIC)
static void check_pic_mask(u16 expected)
{
    assert(!(read_rflags() & RFLAGS_INTERRUPT_ENABLE));
    u16 actual = pic_get_mask();
    if (actual != expected) {
        panic_begin();
        vga_write("ARCH_TEST pic FAIL: expected mask=0x");
        vga_write_hex(expected);
        vga_write(" actual=0x");
        vga_write_hex(actual);
        vga_write("\n");
        panic_halt();
    }
}

static void test_pic_masks(void)
{
    check_pic_mask(0xFFFF);
    assert(pic_unmask_irq(1));
    check_pic_mask(0xFFFD);
    assert(pic_unmask_irq(1));
    check_pic_mask(0xFFFD);
    assert(pic_unmask_irq(0));
    check_pic_mask(0xFFFC);
    assert(pic_mask_irq(1));
    check_pic_mask(0xFFFE);
    assert(pic_mask_irq(0));
    check_pic_mask(0xFFFF);
    assert(pic_mask_irq(0));
    check_pic_mask(0xFFFF);
    assert(pic_unmask_irq(7));
    check_pic_mask(0xFF7F);
    assert(pic_mask_irq(7));
    check_pic_mask(0xFFFF);

    /* Slave changes must preserve other master and slave lines. */
    assert(pic_unmask_irq(1));
    check_pic_mask(0xFFFD);
    assert(pic_unmask_irq(8));
    check_pic_mask(0xFEF9);
    assert(pic_unmask_irq(15));
    check_pic_mask(0x7EF9);
    assert(pic_mask_irq(8));
    check_pic_mask(0x7FF9);
    assert(pic_mask_irq(15));
    check_pic_mask(0xFFFD);
    assert(pic_mask_irq(1));
    check_pic_mask(0xFFFF);

    static const u8 invalid_irqs[] = {PIC_CASCADE_IRQ, PIC_IRQ_COUNT, 255};
    for (size_t i = 0; i < sizeof(invalid_irqs) / sizeof(invalid_irqs[0]); i++) {
        assert(!pic_mask_irq(invalid_irqs[i]));
        check_pic_mask(0xFFFF);
        assert(!pic_unmask_irq(invalid_irqs[i]));
        check_pic_mask(0xFFFF);
    }

    assert(pic_unmask_irq(8));
    check_pic_mask(0xFEFB);
    assert(!pic_mask_irq(PIC_CASCADE_IRQ));
    check_pic_mask(0xFEFB);
    assert(!pic_unmask_irq(PIC_CASCADE_IRQ));
    check_pic_mask(0xFEFB);
    /* Master changes must preserve an open slave cascade. */
    assert(pic_unmask_irq(0));
    check_pic_mask(0xFEFA);
    assert(pic_mask_irq(0));
    check_pic_mask(0xFEFB);
    assert(pic_mask_irq(8));
    check_pic_mask(0xFFFF);
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
#elif defined(ARCH_TEST_PIC)
    test_pic_masks();
    assert(!(read_rflags() & RFLAGS_INTERRUPT_ENABLE));
    vga_write("ARCH_TEST pic PASS\n");
    cpu_halt_forever();
#elif defined(ARCH_TEST_UD)
    show_trigger("ud", arch_test_ud_fault);
    arch_test_ud();
#elif defined(ARCH_TEST_GP)
    check_gp_selector();
    show_trigger("gp", arch_test_gp_fault);
    arch_test_gp(TEST_GP_SELECTOR);
#elif defined(ARCH_TEST_PF)
    check_unmapped_test_page();
    show_trigger("pf", arch_test_pf_fault);
    arch_test_pf(TEST_PF_ADDRESS);
#elif defined(ARCH_TEST_DF)
    check_gp_selector();
    vga_write("ARCH_TEST df: disable GP gate, then load invalid selector\n");
    idt_test_disable_gp_gate();
    arch_test_gp(TEST_GP_SELECTOR);
#elif defined(ARCH_TEST_DF_STACK)
    check_unmapped_test_page();
    vga_write("ARCH_TEST df-stack: push on unmapped stack, RSP=0x");
    vga_write_hex(TEST_DF_STACK_POINTER);
    vga_write("\n");
    arch_test_df_stack(TEST_DF_STACK_POINTER);
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
