#pragma once

/* Early boot only: kernel GDT and VGA ready; interrupts disabled. Call once.
 * Prepares static RSP0/IST1 stacks and loads TR after verifying their contents.
 * Does not switch away from the boot stack or install an interrupt gate.
 */
void tss_init(void);
