/*
 * kernel/include/mm/layout.h
 *
 * PURPOSE:
 * Single source of truth for the kernel's virtual address layout.
 * Only constants and address-conversion macros, no code.
 *
 * LAYOUT:
 *   0xFFFF800000000000  direct map: all physical RAM, virt = phys + DIRECT_MAP_OFFSET
 *   0xFFFFFFFF80000000  kernel image: virt = phys + KERNEL_VIRT_OFFSET
 *
 */

#pragma once

#define KERNEL_VIRT_OFFSET 0xFFFFFFFF80000000ULL
#define DIRECT_MAP_OFFSET 0xFFFF800000000000ULL

// physical memory covered by the boot page tables
#define EARLY_MAP_SIZE (1ULL << 30)

// boot mapping as seen through each higher-half window
#define EARLY_KERNEL_MAP_START KERNEL_VIRT_OFFSET
#define EARLY_KERNEL_MAP_END (KERNEL_VIRT_OFFSET + EARLY_MAP_SIZE)
#define EARLY_DIRECT_MAP_START DIRECT_MAP_OFFSET
#define EARLY_DIRECT_MAP_END (DIRECT_MAP_OFFSET + EARLY_MAP_SIZE)

// Kernel image <-> physical (valid only for addresses inside the kernel image)
#define VIRT_TO_PHYS_KERNEL(vaddr) ((phys_addr_t) (vaddr) - KERNEL_VIRT_OFFSET)
#define PHYS_TO_VIRT_KERNEL(paddr) ((virt_addr_t) (paddr) + KERNEL_VIRT_OFFSET)

// Direct map <-> physical (valid for any physical address inside the direct map)
#define VIRT_TO_PHYS_DIRECT(vaddr) ((phys_addr_t) (vaddr) - DIRECT_MAP_OFFSET)
#define PHYS_TO_VIRT_DIRECT(paddr) ((virt_addr_t) (paddr) + DIRECT_MAP_OFFSET)
