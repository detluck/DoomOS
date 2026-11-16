#include "arch/pic.h"
#include "arch/cpu.h"
#include "arch/io.h"
#include "lib/assert.h"

#define LOG_SCOPE "PIC"
#include "lib/log.h"

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1
#define PIC_INPUT_COUNT 8
#define PIC_MASK_ALL 0xFF
#define PIC_CASCADE_MASK (1U << PIC_CASCADE_IRQ)
#define PIC_ICW1_INIT 0x10
#define PIC_ICW1_ICW4 0x01
#define PIC_ICW4_8086 0x01

static bool initialized;

static void pic_write(u16 port, u8 value)
{
    outb(port, value);
    io_wait();
}

static void pic_check_ready(void)
{
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("PIC: interrupts must be disabled");
    }
    if (!initialized) {
        panic("PIC: not initialized");
    }
}

void pic_init(void)
{
    if (read_rflags() & RFLAGS_INTERRUPT_ENABLE) {
        panic("PIC: interrupts must be disabled before initialization");
    }
    if (initialized) {
        panic("PIC: already initialized");
    }

    pic_write(PIC_MASTER_DATA, PIC_MASK_ALL);
    pic_write(PIC_SLAVE_DATA, PIC_MASK_ALL);
    pic_write(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_write(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_write(PIC_MASTER_DATA, PIC_MASTER_VECTOR_BASE);
    pic_write(PIC_SLAVE_DATA, PIC_SLAVE_VECTOR_BASE);
    pic_write(PIC_MASTER_DATA, PIC_CASCADE_MASK);
    pic_write(PIC_SLAVE_DATA, PIC_CASCADE_IRQ);
    pic_write(PIC_MASTER_DATA, PIC_ICW4_8086);
    pic_write(PIC_SLAVE_DATA, PIC_ICW4_8086);

    /* ICW1 clears the IMRs; do not restore firmware masks. */
    pic_write(PIC_MASTER_DATA, PIC_MASK_ALL);
    pic_write(PIC_SLAVE_DATA, PIC_MASK_ALL);
    u8 master_mask = inb(PIC_MASTER_DATA);
    u8 slave_mask = inb(PIC_SLAVE_DATA);
    if (master_mask != PIC_MASK_ALL || slave_mask != PIC_MASK_ALL) {
        panic("PIC: initial masks did not read back correctly");
    }
    initialized = true;
    pr_ok("PIC initialized; all IRQs masked.");
}

static bool pic_set_irq_mask(u8 irq, bool masked)
{
    pic_check_ready();
    if (irq >= PIC_IRQ_COUNT || irq == PIC_CASCADE_IRQ) {
        return false;
    }

    bool slave = irq >= PIC_INPUT_COUNT;
    u16 port = slave ? PIC_SLAVE_DATA : PIC_MASTER_DATA;
    u8 bit = 1U << (irq % PIC_INPUT_COUNT);
    u8 mask = inb(port);
    if (masked) {
        mask |= bit;
    } else {
        mask &= ~bit;
    }
    pic_write(port, mask);

    if (slave) {
        u8 master_mask = inb(PIC_MASTER_DATA);
        if (mask == PIC_MASK_ALL) {
            master_mask |= PIC_CASCADE_MASK;
        } else {
            master_mask &= ~PIC_CASCADE_MASK;
        }
        pic_write(PIC_MASTER_DATA, master_mask);
    }
    return true;
}

bool pic_mask_irq(u8 irq)
{
    return pic_set_irq_mask(irq, true);
}

bool pic_unmask_irq(u8 irq)
{
    return pic_set_irq_mask(irq, false);
}

u16 pic_get_mask(void)
{
    pic_check_ready();
    u8 master_mask = inb(PIC_MASTER_DATA);
    u8 slave_mask = inb(PIC_SLAVE_DATA);
    return master_mask | ((u16) slave_mask << PIC_INPUT_COUNT);
}
