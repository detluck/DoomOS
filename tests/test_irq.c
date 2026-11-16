#include "arch/cpu.h"
#include "arch/exceptions.h"
#include "arch/io.h"
#include "arch/irq.h"
#include "arch/pic.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "vga.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            _Exit(EXIT_FAILURE); \
        } \
    } while (0)

#define PANIC_EXIT 20
#define EXCEPTION_EXIT 21
#define HALT_EXIT 22
#define TEST_CR2 0x12345000ULL

typedef enum {
    EVENT_READ,
    EVENT_WRITE,
    EVENT_HANDLER,
    EVENT_CR2
} event_kind_t;

typedef struct {
    event_kind_t kind;
    u16 port;
    u8 value;
} event_t;

static event_t events[128];
static size_t event_count;
static size_t event_index;
static u64 cpu_flags;
static const interrupt_context_t *expected_context;
static uintptr_t expected_entry_rsp;
static const char *expected_panic;
static bool expect_exception;
static bool expect_unhandled_vector;
static bool handler_enables_irqs;
static bool panic_started;
static char panic_output[256];

/* Production entry has no C caller in the kernel; the test calls its actual definition. */
extern void interrupt_dispatch(const interrupt_context_t *context, uintptr_t entry_rsp);

static void expect_event(event_kind_t kind, u16 port, u8 value)
{
    CHECK(event_count < sizeof(events) / sizeof(events[0]));
    events[event_count++] = (event_t) {kind, port, value};
}

static event_t next_event(event_kind_t kind, u16 port)
{
    CHECK(event_index < event_count);
    event_t event = events[event_index++];
    CHECK(event.kind == kind);
    CHECK(event.port == port);
    return event;
}

static void finish_events(void)
{
    CHECK(event_index == event_count);
    event_index = 0;
    event_count = 0;
}

static void expect_write(u16 port, u8 value)
{
    expect_event(EVENT_WRITE, port, value);
    expect_event(EVENT_WRITE, 0x80, 0);
}

static void expect_masks(u8 master, u8 slave)
{
    expect_event(EVENT_READ, 0x21, master);
    expect_event(EVENT_READ, 0xA1, slave);
}

u8 inb(u16 port)
{
    return next_event(EVENT_READ, port).value;
}

void outb(u16 port, u8 value)
{
    CHECK(next_event(EVENT_WRITE, port).value == value);
}

u64 read_rflags(void)
{
    return cpu_flags;
}

u64 read_cr2(void)
{
    next_event(EVENT_CR2, 0);
    return TEST_CR2;
}

void log_ok(const char *scope, const char *fmt, ...)
{
    CHECK(strcmp(scope, "PIC") == 0);
    CHECK(strcmp(fmt, "PIC initialized; all IRQs masked.") == 0);
    CHECK(event_index == event_count);
}

void log_warn(const char *scope, const char *fmt, ...)
{
    (void) scope;
    (void) fmt;
    CHECK(false);
}

void panic(const char *message)
{
    CHECK(expected_panic != NULL);
    CHECK(strcmp(message, expected_panic) == 0);
    finish_events();
    _Exit(PANIC_EXIT);
}

void panic_begin(void)
{
    CHECK(expect_unhandled_vector);
    CHECK(!panic_started);
    panic_started = true;
}

void vga_write(const char *text)
{
    CHECK(panic_started);
    CHECK(strlen(panic_output) + strlen(text) < sizeof(panic_output));
    strcat(panic_output, text);
}

void vga_write_hex(u64 value)
{
    char text[17];
    snprintf(text, sizeof(text), "%llX", (unsigned long long) value);
    vga_write(text);
}

void panic_halt(void)
{
    CHECK(panic_started);
    CHECK(strcmp(panic_output, "Unhandled interrupt: vector=0x30 error=0x0\nSystem halted.\n") ==
          0);
    finish_events();
    _Exit(HALT_EXIT);
}

void exception_panic(const interrupt_context_t *context, u64 fault_address, uintptr_t entry_rsp)
{
    CHECK(expect_exception);
    CHECK(context == expected_context);
    CHECK(entry_rsp == expected_entry_rsp);
    CHECK(fault_address == (context->vector == EXCEPTION_PAGE_FAULT ? TEST_CR2 : 0));
    finish_events();
    _Exit(EXCEPTION_EXIT);
}

static void handler(const interrupt_context_t *context)
{
    next_event(EVENT_HANDLER, 0);
    CHECK(context == expected_context);
    CHECK(!(cpu_flags & RFLAGS_INTERRUPT_ENABLE));
    if (handler_enables_irqs) {
        cpu_flags |= RFLAGS_INTERRUPT_ENABLE;
    }
}

static void other_handler(const interrupt_context_t *context)
{
    (void) context;
    CHECK(false);
}

static void init_pic(void)
{
    /* Actual ICW sequence, not a second implementation of the controller. */
    static const struct {
        u16 port;
        u8 value;
    } writes[] = {
        {0x21, 0xFF}, {0xA1, 0xFF}, {0x20, 0x11}, {0xA0, 0x11}, {0x21, 0x20}, {0xA1, 0x28},
        {0x21, 0x04}, {0xA1, 0x02}, {0x21, 0x01}, {0xA1, 0x01}, {0x21, 0xFF}, {0xA1, 0xFF},
    };
    for (size_t i = 0; i < sizeof(writes) / sizeof(writes[0]); i++) {
        expect_write(writes[i].port, writes[i].value);
    }
    expect_masks(0xFF, 0xFF);
    pic_init();
    finish_events();
}

static void register_handler(u8 irq)
{
    expect_masks(0xFF, 0xFF);
    CHECK(irq_register_handler(irq, handler));
    finish_events();
}

static void expect_isr(u16 port, u8 value)
{
    expect_write(port, 0x0B);
    expect_event(EVENT_READ, port, value);
}

static void dispatch(u64 vector, bool via_entry)
{
    interrupt_context_t context;
    memset(&context, 0xA5, sizeof(context));
    context.vector = vector;
    context.error_code = 0;
    interrupt_context_t original = context;
    expected_context = &context;
    if (via_entry) {
        /* Synthetic address for the C guard; not a test of the actual ISR stack. */
        uintptr_t entry_rsp = ((uintptr_t) &context & ~(uintptr_t) 15) - 8;
        interrupt_dispatch(&context, entry_rsp);
    } else {
        CHECK(irq_dispatch(&context));
    }
    CHECK(memcmp(&context, &original, sizeof(context)) == 0);
    CHECK(!(cpu_flags & RFLAGS_INTERRUPT_ENABLE));
    finish_events();
}

static void test_registration(void)
{
    init_pic();
    CHECK(irq_get_unhandled_mask() == 0);
    CHECK(!irq_register_handler(2, handler));
    CHECK(!irq_register_handler(16, handler));
    CHECK(!irq_register_handler(255, handler));
    CHECK(!irq_register_handler(1, NULL));
    for (u8 irq = 0; irq < 16; irq++) {
        if (irq != 2) {
            register_handler(irq);
            CHECK(!irq_register_handler(irq, handler));
            CHECK(!irq_register_handler(irq, other_handler));
        }
    }
    /* Rejections must not replace the original handler. */
    expect_event(EVENT_HANDLER, 0, 0);
    expect_write(0x20, 0x20);
    dispatch(33, false);
    CHECK(irq_get_unhandled_mask() == 0);
}

static void test_registration_masks(void)
{
    init_pic();
    expect_masks(0xFD, 0xFF);
    CHECK(!irq_register_handler(1, handler));
    finish_events();
    register_handler(1);
    expect_masks(0xFB, 0x7F);
    CHECK(!irq_register_handler(15, handler));
    finish_events();
    /* IRQ8 masked while the slave cascade and IRQ15 are open. */
    expect_masks(0xFB, 0x7F);
    CHECK(irq_register_handler(8, handler));
    finish_events();
}

static void test_vector_bounds(void)
{
    static const u64 vectors[] = {0, 31, 48, 255, 0x120, UINT64_MAX};
    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        interrupt_context_t context = {.vector = vectors[i]};
        CHECK(!irq_dispatch(&context));
    }
    finish_events();
}

static void test_registered_delivery(void)
{
    init_pic();
    for (u8 irq = 0; irq < 16; irq++) {
        if (irq == 2) {
            continue;
        }
        register_handler(irq);
        if (irq == 7 || irq == 15) {
            expect_isr(irq == 7 ? 0x20 : 0xA0, 0x81);
        }
        expect_event(EVENT_HANDLER, 0, 0);
        if (irq >= 8) {
            expect_write(0xA0, 0x20);
        }
        expect_write(0x20, 0x20);
        dispatch(32 + irq, true);
        CHECK(irq_get_unhandled_mask() == 0);
    }
}

static void test_spurious(void)
{
    init_pic();
    register_handler(7);
    register_handler(15);
    static const u8 isr_values[] = {0, 1, 0x80, 0xFF, 0};
    for (size_t i = 0; i < sizeof(isr_values) / sizeof(isr_values[0]); i++) {
        u8 isr = isr_values[i];
        expect_isr(0x20, isr);
        if (isr & 0x80) {
            expect_event(EVENT_HANDLER, 0, 0);
            expect_write(0x20, 0x20);
        }
        dispatch(39, true);
        expect_isr(0xA0, isr);
        if (isr & 0x80) {
            expect_event(EVENT_HANDLER, 0, 0);
            expect_write(0xA0, 0x20);
        }
        expect_write(0x20, 0x20);
        dispatch(47, true);
        CHECK(irq_get_unhandled_mask() == 0);
    }
}

static void test_unregistered_spurious(void)
{
    init_pic();
    expect_isr(0x20, 0);
    dispatch(39, false);
    expect_isr(0xA0, 1);
    expect_write(0x20, 0x20);
    dispatch(47, false);
    CHECK(irq_get_unhandled_mask() == 0);
}

static void test_unhandled(void)
{
    init_pic();
    /* IRQ1 is closed; other master lines stay as they were. */
    expect_event(EVENT_READ, 0x21, 0xF9);
    expect_write(0x21, 0xFB);
    expect_write(0x20, 0x20);
    dispatch(33, true);
    CHECK(irq_get_unhandled_mask() == 0x0002);
    /* A repeated event does not clear the latched diagnosis or print anything. */
    expect_event(EVENT_READ, 0x21, 0xFB);
    expect_write(0x21, 0xFB);
    expect_write(0x20, 0x20);
    dispatch(33, false);
    CHECK(irq_get_unhandled_mask() == 0x0002);

    /* Closing IRQ8 preserves active IRQ15 and its cascade. */
    expect_event(EVENT_READ, 0xA1, 0x7E);
    expect_write(0xA1, 0x7F);
    expect_event(EVENT_READ, 0x21, 0xFB);
    expect_write(0x21, 0xFB);
    expect_write(0xA0, 0x20);
    expect_write(0x20, 0x20);
    dispatch(40, false);
    CHECK(irq_get_unhandled_mask() == 0x0102);

    /* Closing the final slave line still requires both EOIs. */
    expect_isr(0xA0, 0x80);
    expect_event(EVENT_READ, 0xA1, 0x7F);
    expect_write(0xA1, 0xFF);
    expect_event(EVENT_READ, 0x21, 0xFB);
    expect_write(0x21, 0xFF);
    expect_write(0xA0, 0x20);
    expect_write(0x20, 0x20);
    dispatch(47, false);
    CHECK(irq_get_unhandled_mask() == 0x8102);
    CHECK(irq_get_unhandled_mask() == 0x8102);

    expect_isr(0x20, 0x80);
    expect_event(EVENT_READ, 0x21, 0x7F);
    expect_write(0x21, 0xFF);
    expect_write(0x20, 0x20);
    dispatch(39, false);
    CHECK(irq_get_unhandled_mask() == 0x8182);

    /* Later spurious/handled events must not alter previous fault history. */
    expect_isr(0xA0, 0);
    expect_write(0x20, 0x20);
    dispatch(47, false);
    register_handler(1);
    expect_event(EVENT_HANDLER, 0, 0);
    expect_write(0x20, 0x20);
    dispatch(33, false);
    CHECK(irq_get_unhandled_mask() == 0x8182);
}

static void test_registration_before_init(void)
{
    expected_panic = "PIC: not initialized";
    irq_register_handler(1, handler);
}

static void test_dispatch_before_init(void)
{
    expected_panic = "PIC: not initialized";
    dispatch(33, false);
}

static void test_duplicate_init(void)
{
    init_pic();
    expected_panic = "PIC: already initialized";
    pic_init();
}

static void test_registration_if(void)
{
    init_pic();
    cpu_flags = RFLAGS_INTERRUPT_ENABLE;
    expected_panic = "IRQ: interrupts must be disabled";
    irq_register_handler(1, handler);
}

static void test_dispatch_if(void)
{
    init_pic();
    cpu_flags = RFLAGS_INTERRUPT_ENABLE;
    expected_panic = "IRQ: interrupts must be disabled";
    dispatch(33, false);
}

static void test_diagnostic_if(void)
{
    cpu_flags = RFLAGS_INTERRUPT_ENABLE;
    expected_panic = "IRQ: interrupts must be disabled";
    irq_get_unhandled_mask();
}

static void test_handler_if(void)
{
    init_pic();
    register_handler(1);
    handler_enables_irqs = true;
    expected_panic = "IRQ: interrupts must be disabled";
    expect_event(EVENT_HANDLER, 0, 0);
    dispatch(33, false);
}

static void test_cascade(void)
{
    init_pic();
    expected_panic = "IRQ: unexpected cascade vector";
    dispatch(34, true);
}

static void test_pic_begin_invalid(void)
{
    init_pic();
    expected_panic = "PIC: invalid device IRQ";
    pic_begin_irq(2);
}

static void test_pic_end_invalid(void)
{
    init_pic();
    expected_panic = "PIC: invalid device IRQ";
    pic_end_irq(16);
}

static void test_pic_begin_if(void)
{
    init_pic();
    cpu_flags = RFLAGS_INTERRUPT_ENABLE;
    expected_panic = "PIC: interrupts must be disabled";
    pic_begin_irq(7);
}

static void test_pic_end_if(void)
{
    init_pic();
    cpu_flags = RFLAGS_INTERRUPT_ENABLE;
    expected_panic = "PIC: interrupts must be disabled";
    pic_end_irq(8);
}

static void test_pic_end_before_init(void)
{
    expected_panic = "PIC: not initialized";
    pic_end_irq(1);
}

static void exception_entry(u64 vector)
{
    interrupt_context_t context = {.vector = vector};
    expected_context = &context;
    expected_entry_rsp = ((uintptr_t) &context & ~(uintptr_t) 15) - 8;
    expect_exception = true;
    if (vector == EXCEPTION_PAGE_FAULT) {
        expect_event(EVENT_CR2, 0, 0);
    }
    interrupt_dispatch(&context, expected_entry_rsp);
}

static void test_page_fault(void)
{
    exception_entry(EXCEPTION_PAGE_FAULT);
}

static void test_general_protection(void)
{
    exception_entry(EXCEPTION_GENERAL_PROTECTION);
}

static void test_invalid_entry(void)
{
    interrupt_context_t context = {.vector = EXCEPTION_PAGE_FAULT};
    expect_event(EVENT_CR2, 0, 0);
    expected_panic = "ISR: invalid C entry stack";
    interrupt_dispatch(&context, 0);
}

static void test_other_vector(void)
{
    interrupt_context_t context = {.vector = 48};
    uintptr_t entry_rsp = ((uintptr_t) &context & ~(uintptr_t) 15) - 8;
    expect_unhandled_vector = true;
    interrupt_dispatch(&context, entry_rsp);
}

int main(void)
{
    static const struct {
        const char *name;
        void (*run)(void);
        int exit_status;
    } tests[] = {
        {"registration", test_registration, 0},
        {"registration masks", test_registration_masks, 0},
        {"vector bounds", test_vector_bounds, 0},
        {"registered delivery", test_registered_delivery, 0},
        {"spurious and real IRQ7/15", test_spurious, 0},
        {"unregistered spurious", test_unregistered_spurious, 0},
        {"unhandled IRQs and diagnosis", test_unhandled, 0},
        {"registration before init", test_registration_before_init, PANIC_EXIT},
        {"dispatch before init", test_dispatch_before_init, PANIC_EXIT},
        {"duplicate PIC init", test_duplicate_init, PANIC_EXIT},
        {"registration with IF set", test_registration_if, PANIC_EXIT},
        {"dispatch with IF set", test_dispatch_if, PANIC_EXIT},
        {"diagnosis with IF set", test_diagnostic_if, PANIC_EXIT},
        {"handler enables IRQs", test_handler_if, PANIC_EXIT},
        {"cascade vector", test_cascade, PANIC_EXIT},
        {"PIC begin invalid IRQ", test_pic_begin_invalid, PANIC_EXIT},
        {"PIC end invalid IRQ", test_pic_end_invalid, PANIC_EXIT},
        {"PIC begin with IF set", test_pic_begin_if, PANIC_EXIT},
        {"PIC end with IF set", test_pic_end_if, PANIC_EXIT},
        {"PIC end before init", test_pic_end_before_init, PANIC_EXIT},
        {"page fault dispatch", test_page_fault, EXCEPTION_EXIT},
        {"general protection dispatch", test_general_protection, EXCEPTION_EXIT},
        {"invalid entry after CR2 capture", test_invalid_entry, PANIC_EXIT},
        {"other vector panic", test_other_vector, HALT_EXIT},
    };
    /* The parent never initializes production globals; each child starts with fresh BSS. */
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        fflush(NULL);
        pid_t child = fork();
        CHECK(child >= 0);
        if (child == 0) {
            tests[i].run();
            finish_events();
            _Exit(EXIT_SUCCESS);
        }
        int status;
        pid_t waited;
        do {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);
        CHECK(waited == child);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != tests[i].exit_status) {
            fprintf(stderr, "[FAIL] %s: wait status %d, expected exit %d\n", tests[i].name, status,
                    tests[i].exit_status);
            return EXIT_FAILURE;
        }
        printf("[PASS] %s\n", tests[i].name);
    }
    return EXIT_SUCCESS;
}
