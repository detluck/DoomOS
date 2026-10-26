#pragma once

#include "types.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT) // 4 KiB
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PFN_TO_PHYS(pfn) ((phys_addr_t) (pfn) << PAGE_SHIFT)
#define PHYS_TO_PFN(phys) ((pfn_t) (phys) >> PAGE_SHIFT)

void pmm_init(void);

phys_addr_t pmm_alloc_frame(void);

void pmm_free_frame(phys_addr_t phys_addr);

u64 pmm_get_total_frames(void);
u64 pmm_get_free_frames(void);
