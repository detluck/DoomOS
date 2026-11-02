#include "mm/vmm.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "types.h"
#include "lib/assert.h"
#define LOG_SCOPE "VMM"
#include "lib/log.h"

static pml4_t *kernel_pml4 = NULL;

static page_table_t *get_or_create_table(page_table_t *table, size_t index, bool allocate,
                                         u64 flags)
{
    pt_entry_t entry = table->entries[index];
    if (entry & VMM_FLAG_PRESENT) {
        phys_addr_t paddr = entry & PTE_ADDR_MASK;
        return (page_table_t *) paddr;
    } else {
        if (!allocate) {
            return NULL;
        } else {
            phys_addr_t paddr = pmm_alloc_frame();
            if (paddr == 0) {
                return NULL;
            }
            memset((void *) paddr, 0, sizeof(page_table_t));
            table->entries[index] =
                paddr | VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE | (flags & VMM_FLAG_USER);
            return (page_table_t *) paddr;
        }
    }
}

static pt_entry_t *get_pte(pml4_t *pml4, virt_addr_t vaddr, bool allocate, u64 flags)
{
    size_t pml4_i = vmm_pml4_index(vaddr);
    pdpt_t *pdpt = get_or_create_table(pml4, pml4_i, allocate, flags);
    if (!pdpt) {
        return NULL;
    }
    size_t pdpt_i = vmm_pdpt_index(vaddr);
    pd_t *pd = get_or_create_table(pdpt, pdpt_i, allocate, flags);
    if (!pd) {
        return NULL;
    }
    size_t pd_i = vmm_pd_index(vaddr);
    pt_t *pt = get_or_create_table(pd, pd_i, allocate, flags);
    if (!pt) {
        return NULL;
    }
    size_t pt_i = vmm_pt_index(vaddr);
    return &pt->entries[pt_i];
}

bool vmm_map_page(pml4_t *pml4, virt_addr_t vaddr, phys_addr_t paddr, u64 flags)
{
    // check 4-Kib alignment
    if (vaddr & PAGE_OFFSET_MASK || paddr & PAGE_OFFSET_MASK) {
        pr_err("Addresses must be 4-Kib aligned");
        return false;
    }

    pt_entry_t *pte = get_pte(pml4, vaddr, true, flags);
    if (!pte) {
        return false;
    }

    *pte = (paddr & PTE_ADDR_MASK) | flags | VMM_FLAG_PRESENT;

    vmm_invlpg(vaddr);
    return true;
}

bool vmm_map_range(pml4_t *pml4, virt_addr_t vstart, phys_addr_t pstart, size_t size, u64 flags)
{
    for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        if (!vmm_map_page(pml4, vstart + offset, pstart + offset, flags)) {
            return false;
        }
    }
    return true;
}

void vmm_unmap_page(pml4_t *pml4, virt_addr_t vaddr)
{
    pt_entry_t *pte = get_pte(pml4, vaddr, false, 0);

    if (!pte || !(*pte & VMM_FLAG_PRESENT)) {
        return;
    }
    *pte = 0;
    vmm_invlpg(vaddr);
}

bool vmm_is_mapped(pml4_t *pml4, virt_addr_t vaddr)
{
    if (vaddr & PAGE_OFFSET_MASK) {
        return false;
    }
    const pt_entry_t *pte = get_pte(pml4, vaddr, false, 0);
    return (pte != NULL) && (*pte & VMM_FLAG_PRESENT);
}

phys_addr_t vmm_virt_to_phys(pml4_t *pml4, virt_addr_t vaddr)
{
    const pt_entry_t *pte = get_pte(pml4, vaddr, false, 0);
    if (!pte || !(*pte & VMM_FLAG_PRESENT)) {
        return 0;
    }
    return (*pte & PTE_ADDR_MASK) | vmm_page_offset(vaddr);
}

void vmm_init(void)
{
    phys_addr_t pml4_phys = pmm_alloc_frame();
    kernel_pml4 = (pml4_t *) pml4_phys;
    memset(kernel_pml4, 0, sizeof(pml4_t));

    phys_addr_t ram_size = PFN_TO_PHYS(pmm_get_total_frames());

    // 1. MUST MAP LOW MEMORY (keeps RIP at 0x101c8c alive after mov cr3!):
    vmm_map_range(kernel_pml4, 0x0, 0x0, ram_size, VMM_FLAG_WRITABLE);

    // 2. Map the Linux Direct Map (0xFFFF8000...):
    vmm_map_range(kernel_pml4, DIRECT_MAP_OFFSET, 0x0, ram_size, VMM_FLAG_WRITABLE);

    vmm_switch_pml4(pml4_phys);

    // 3. Update pointer to use the direct map:
    kernel_pml4 = (pml4_t *) PHYS_TO_VIRT_DIRECT(pml4_phys);
    pr_ok("VMM is initialized with Linux Direct Map");
}

void vmm_test(void)
{

    pr_info("Starting VMM self-test...");

    virt_addr_t test_vaddr = 0x10000000;
    phys_addr_t test_frame = pmm_alloc_frame();
    // check if the address not mapped yet
    assert(!(vmm_is_mapped(kernel_pml4, test_vaddr)));
    // map them
    assert(vmm_map_page(kernel_pml4, test_vaddr, test_frame, VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE));
    // check if mapping was successfull
    assert(vmm_is_mapped(kernel_pml4, test_vaddr));
    assert(vmm_virt_to_phys(kernel_pml4, test_vaddr) == test_frame);
    // chcke if MMU translates correctly
    // write through the virtual pointer
    volatile u64 *virt_ptr = (volatile u64 *) test_vaddr;
    *virt_ptr = 0xCAFEBABE12345678ULL;
    // read through the physical pointer
    const volatile u64 *phys_ptr = (volatile u64 *) test_frame;
    assert(*phys_ptr == 0xCAFEBABE12345678ULL);
    // unmap and clean up
    vmm_unmap_page(kernel_pml4, test_vaddr);
    assert(!vmm_is_mapped(kernel_pml4, test_vaddr));
    pmm_free_frame(test_frame);
    pr_ok("VMM self-test passed successfully!");
}
