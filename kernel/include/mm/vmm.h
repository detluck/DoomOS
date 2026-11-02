#pragma once
#include "types.h"
#include "defs.h"
#include "mm/pmm.h"
#include <stddef.h>

#define PAGE_TABLE_ENTRIES 512
#define PAGE_OFFSET_MASK (PAGE_SIZE - 1)
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define DIRECT_MAP_OFFSET 0xFFFF800000000000
#define KERNEL_VIRT_OFFSET 0xFFFFFFFF80000000ULL

// macros for kernel memory only
#define VIRT_TO_PHYS_DIRECT(vaddr) ((phys_addr_t) (vaddr) - DIRECT_MAP_OFFSET)
#define PHYS_TO_VIRT_DIRECT(paddr) ((virt_addr_t) (paddr) + DIRECT_MAP_OFFSET)

// avl flags macro
#define VMM_AVL_SHIFT 9
#define VMM_AVL_MASK (0x7ULL << VMM_AVL_SHIFT)
#define VMM_AVL_VALUE(val) (((u64) val & 0x7ULL) << VMM_AVL_SHIFT)
#define VMM_AVL_GET(entry) (((u64) (entry) >> VMM_AVL_SHIFT) & 0x7ULL)
// flags
#define VMM_FLAG_PRESENT (1ULL << 0)
#define VMM_FLAG_WRITABLE (1ULL << 1)
#define VMM_FLAG_USER (1ULL << 2)
#define VMM_FLAG_WRITE_THROUGH (1ULL << 3)
#define VMM_FLAG_CACHE_DISABLED (1ULL << 4)
#define VMM_FLAG_ACCESSED (1ULL << 5)
#define VMM_FLAG_DIRTY (1ULL << 6)
#define VMM_FLAG_HUGE_PAGE (1ULL << 7)
#define VMM_FLAG_GLOBAL (1ULL << 8)
#define VMM_FLAG_NO_EXECUTE (1ULL << 63)

typedef u64 pt_entry_t;

typedef struct page_table {
    pt_entry_t entries[PAGE_TABLE_ENTRIES];
} __aligned(4096) page_table_t;

// 4-Level Paging
typedef page_table_t pml4_t;
typedef page_table_t pdpt_t;
typedef page_table_t pd_t;
typedef page_table_t pt_t;

static __always_inline size_t vmm_pml4_index(virt_addr_t vaddr)
{
    return (vaddr >> 39) & 0x1FF;
}

static __always_inline size_t vmm_pdpt_index(virt_addr_t vaddr)
{
    return (vaddr >> 30) & 0x1FF;
}

static __always_inline size_t vmm_pd_index(virt_addr_t vaddr)
{
    return (vaddr >> 21) & 0x1FF;
}

static __always_inline size_t vmm_pt_index(virt_addr_t vaddr)
{
    return (vaddr >> 12) & 0x1FF;
}

static __always_inline size_t vmm_page_offset(virt_addr_t vaddr)
{
    return vaddr & 0xFFF;
}

static __always_inline void vmm_invlpg(virt_addr_t vaddr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static __always_inline void vmm_switch_pml4(phys_addr_t paddr)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(paddr) : "memory");
}

void vmm_init(void);
void vmm_unmap_page(pml4_t *pml4, virt_addr_t vaddr);
void vmm_test(void);
bool vmm_map_page(pml4_t *pml4, virt_addr_t vaddr, phys_addr_t paddr, u64 flags);
bool vmm_map_range(pml4_t *pml4, virt_addr_t vstart, phys_addr_t pstart, size_t size, u64 flags);
bool vmm_is_mapped(pml4_t *pml4, virt_addr_t vaddr);
phys_addr_t vmm_virt_to_phys(pml4_t *pml4, virt_addr_t vaddr);
