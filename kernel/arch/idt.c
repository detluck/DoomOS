#include "idt.h"
#include "arch/boot.h"
#include "arch/cpu.h"
#include "defs.h"
#include "gdt.h"
#include "lib/assert.h"
#include "types.h"

#define LOG_SCOPE "IDT"
#include "lib/log.h"

#define IDT_ENTRY_COUNT 256
#define IDT_GATE_ATTRIBUTES 0x8E

typedef struct {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 attributes;
    u16 offset_middle;
    u32 offset_high;
    u32 reserved;
} __packed idt_entry_t;

typedef struct {
    u16 limit;
    u64 base;
} __packed idt_pointer_t;

extern const uintptr_t isr_stub_table[IDT_ENTRY_COUNT];
extern const u8 isr_code_start[], isr_code_end[];
extern const u8 _kernel_start[], _kernel_end[];

static idt_entry_t idt[IDT_ENTRY_COUNT] __aligned(16);
static const idt_pointer_t idt_pointer = {
    .limit = sizeof(idt) - 1,
    .base = (uintptr_t) idt,
};
static bool initialized;

_Static_assert(sizeof(idt_entry_t) == 16, "IDT gate must be 16 bytes");
_Static_assert(offsetof(idt_entry_t, selector) == 2, "IDT selector offset");
_Static_assert(offsetof(idt_entry_t, ist) == 4, "IDT IST offset");
_Static_assert(offsetof(idt_entry_t, attributes) == 5, "IDT attributes offset");
_Static_assert(offsetof(idt_entry_t, offset_middle) == 6, "IDT middle offset");
_Static_assert(offsetof(idt_entry_t, offset_high) == 8, "IDT high offset");
_Static_assert(offsetof(idt_entry_t, reserved) == 12, "IDT reserved offset");
_Static_assert(sizeof(idt_pointer_t) == 10, "IDTR operand must be 10 bytes");
_Static_assert(offsetof(idt_pointer_t, base) == 2, "IDTR base offset");
_Static_assert(sizeof(idt) == 4096, "IDT must contain 256 gates");

static inline idt_pointer_t read_idtr(void)
{
    idt_pointer_t pointer;
    __asm__ volatile("sidt %0" : "=m"(pointer) : : "memory");
    return pointer;
}

static inline void load_idtr(const idt_pointer_t *pointer)
{
    __asm__ volatile("lidt %0" : : "m"(*pointer) : "memory");
}

static inline u16 read_code_selector(void)
{
    u16 selector;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    return selector;
}

static inline u16 read_task_register(void)
{
    u16 selector;
    __asm__ volatile("str %0" : "=m"(selector));
    return selector;
}

static void idt_check_region(uintptr_t base, size_t size)
{
    if (base < EARLY_KERNEL_MAP_START || base >= EARLY_KERNEL_MAP_END ||
        size > EARLY_KERNEL_MAP_END - base) {
        panic("IDT: required memory outside early kernel mapping");
    }
    uintptr_t kernel_start = (uintptr_t) _kernel_start;
    uintptr_t kernel_end = (uintptr_t) _kernel_end;
    if (base < kernel_start || base >= kernel_end || size > kernel_end - base) {
        panic("IDT: required memory outside reserved kernel");
    }
}

static void idt_check_gates(void)
{
    const volatile idt_entry_t *entries = idt;
    for (size_t i = 0; i < IDT_ENTRY_COUNT; i++) {
        u64 target = entries[i].offset_low | ((u64) entries[i].offset_middle << 16) |
                     ((u64) entries[i].offset_high << 32);
        if (target != isr_stub_table[i] || entries[i].selector != GDT_KERNEL_CODE_SELECTOR ||
            entries[i].ist != 0 || entries[i].attributes != IDT_GATE_ATTRIBUTES ||
            entries[i].reserved != 0) {
            panic("IDT: invalid interrupt gate");
        }
    }
}

/* Read-only verification of active IDTR, IF=0 and gate encodings (e.g. after CR3 switch). */
void idt_check_active(void)
{
    idt_pointer_t loaded = read_idtr();
    if (!initialized || loaded.base != (uintptr_t) idt || loaded.limit != sizeof(idt) - 1) {
        panic("IDT: unexpected IDTR");
    }
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("IDT: interrupts unexpectedly enabled");
    }
    idt_check_gates();
}

void idt_init(void)
{
    if (initialized) {
        panic("IDT: already initialized");
    }
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("IDT: interrupts must be disabled before initialization");
    }
    u16 code_selector = read_code_selector();
    u16 task_selector = read_task_register();
    if (code_selector != GDT_KERNEL_CODE_SELECTOR || task_selector != GDT_TSS_SELECTOR) {
        panic("IDT: kernel GDT and TSS must be active");
    }

    idt_check_region((uintptr_t) idt, sizeof(idt));
    idt_check_region((uintptr_t) &idt_pointer, sizeof(idt_pointer));
    idt_check_region((uintptr_t) isr_stub_table, sizeof(isr_stub_table));
    uintptr_t code_start = (uintptr_t) isr_code_start;
    uintptr_t code_end = (uintptr_t) isr_code_end;
    if (code_end <= code_start) {
        panic("IDT: invalid ISR code range");
    }
    idt_check_region(code_start, code_end - code_start);
    for (size_t i = 0; i < IDT_ENTRY_COUNT; i++) {
        uintptr_t target = isr_stub_table[i];
        if (target < code_start || target >= code_end) {
            panic("IDT: stub target outside ISR code");
        }
        idt[i] = (idt_entry_t) {
            .offset_low = (u16) target,
            .selector = GDT_KERNEL_CODE_SELECTOR,
            .ist = 0,
            .attributes = IDT_GATE_ATTRIBUTES,
            .offset_middle = (u16) (target >> 16),
            .offset_high = (u32) (target >> 32),
            .reserved = 0,
        };
    }
    idt_check_gates();
    load_idtr(&idt_pointer);
    initialized = true;
    idt_check_active();
    pr_ok("IDT initialized and verified.");
}
