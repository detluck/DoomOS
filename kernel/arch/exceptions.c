#include "arch/exceptions.h"
#include "lib/assert.h"
#include "vga.h"

#define EXCEPTION_GENERAL_PROTECTION 13
#define PAGE_FAULT_PRESENT (1ULL << 0)
#define PAGE_FAULT_WRITE (1ULL << 1)
#define PAGE_FAULT_USER (1ULL << 2)
#define PAGE_FAULT_RESERVED (1ULL << 3)
#define PAGE_FAULT_INSTRUCTION (1ULL << 4)
#define SELECTOR_ERROR_EXTERNAL (1ULL << 0)
#define SELECTOR_ERROR_IDT (1ULL << 1)
#define SELECTOR_ERROR_LDT (1ULL << 2)
#define SELECTOR_ERROR_INDEX_SHIFT 3
#define SELECTOR_ERROR_INDEX_MASK 0x1FFF

static const char *const exception_names[EXCEPTION_COUNT] = {
    "#DE Divide Error",
    "#DB Debug",
    "NMI",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR Bound Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "Reserved (legacy vector 9)",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection",
    "#PF Page Fault",
    "Reserved",
    "#MF x87 Floating Point",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XM SIMD Floating Point",
    "#VE Virtualization",
    "#CP Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "#HV Hypervisor Injection",
    "#VC VMM Communication",
    "#SX Security",
    "Reserved",
};

static void write_value(const char *name, u64 value)
{
    vga_write(name);
    vga_write("=0x");
    vga_write_hex(value);
}

static void write_pair(const char *first_name, u64 first, const char *second_name, u64 second)
{
    write_value(first_name, first);
    vga_write("  ");
    write_value(second_name, second);
    vga_write("\n");
}

static void dump_context(const interrupt_context_t *context)
{
    write_pair("Vector", context->vector, "Error", context->error_code);
    write_pair("RIP", context->rip, "CS", context->cs);
    write_value("RFLAGS", context->rflags);
    vga_write("\n");
    write_pair("Interrupted RSP", context->rsp, "SS", context->ss);
    write_pair("RAX", context->rax, "RBX", context->rbx);
    write_pair("RCX", context->rcx, "RDX", context->rdx);
    write_pair("RSI", context->rsi, "RDI", context->rdi);
    write_pair("RBP", context->rbp, "R8", context->r8);
    write_pair("R9", context->r9, "R10", context->r10);
    write_pair("R11", context->r11, "R12", context->r12);
    write_pair("R13", context->r13, "R14", context->r14);
    write_value("R15", context->r15);
    vga_write("\n");
}

static void dump_page_fault(u64 error_code, u64 fault_address)
{
    write_value("CR2", fault_address);
    vga_write("\nPF: ");
    vga_write(error_code & PAGE_FAULT_PRESENT ? "protection violation" : "non-present page");
    vga_write(error_code & PAGE_FAULT_WRITE ? ", write" : ", read");
    vga_write(error_code & PAGE_FAULT_USER ? ", user\n" : ", supervisor\n");
    vga_write(error_code & PAGE_FAULT_RESERVED ? "RSVD=1 (reserved bit violation)" : "RSVD=0");
    vga_write(error_code & PAGE_FAULT_INSTRUCTION ? "  I/D=1 (instruction fetch)\n"
                                                  : "  I/D=0 (data access)\n");
}

static void dump_general_protection(u64 error_code)
{
    if (error_code == 0) {
        vga_write("GP: no selector information\n");
        return;
    }
    vga_write(error_code & SELECTOR_ERROR_EXTERNAL ? "GP: EXT=1  table=" : "GP: EXT=0  table=");
    if (error_code & SELECTOR_ERROR_IDT) {
        vga_write("IDT");
    } else {
        vga_write(error_code & SELECTOR_ERROR_LDT ? "LDT" : "GDT");
    }
    vga_write("  ");
    write_value("index", (error_code >> SELECTOR_ERROR_INDEX_SHIFT) & SELECTOR_ERROR_INDEX_MASK);
    vga_write("\n");
}

void exception_panic(const interrupt_context_t *context, u64 fault_address)
{
    panic_begin();
    vga_write("Exception: ");
    vga_write(context->vector < EXCEPTION_COUNT ? exception_names[context->vector] : "Unknown");
    vga_write("\n");
    dump_context(context);
    if (context->vector == EXCEPTION_PAGE_FAULT) {
        dump_page_fault(context->error_code, fault_address);
    } else if (context->vector == EXCEPTION_GENERAL_PROTECTION) {
        dump_general_protection(context->error_code);
    }
    vga_write("System halted.\n");
    panic_halt();
}
