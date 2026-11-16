#pragma once

/* Early boot, once: kernel GDT/TSS and VGA ready, valid stack, IF=0.
 * Installs all 256 interrupt gates without enabling interrupts. Only #DF uses IST1.
 */
void idt_init(void);

/* Read-only verification that the kernel IDT is still active, IF=0 and all gates match. */
void idt_check_active(void);

#ifdef ARCH_TEST_DF
/* Fatal test only: verifies the active IDT, then clears the #GP gate's present bit. */
void idt_test_disable_gp_gate(void);
#endif
