#pragma once

/* Early boot, once: kernel GDT/TSS and VGA ready, valid stack, IF=0.
 * Installs all 256 interrupt gates without enabling interrupts. IST remains 0.
 */
void idt_init(void);

/* Read-only verification that the kernel IDT is still active, IF=0 and all gates match. */
void idt_check_active(void);
