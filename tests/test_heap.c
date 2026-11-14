#include "mm/heap.h"
#include "lib/string.h"
#include "lib/kprintf.h"
#include "lib/assert.h"

void test_kmalloc_boundaries(void)
{
    void *p1 = kmalloc(KMALLOC_MAX_SIZE);
    assert(p1 != NULL);
    assert(ksize(p1) == KMALLOC_MAX_SIZE);

    // Min large alloc size
    void *p2 = kmalloc(KMALLOC_MAX_SIZE + 1);
    assert(p2 != NULL);
    assert(ksize(p2) >= KMALLOC_MAX_SIZE + 1);

    kfree(p1);
    kfree(p2);

    kprintf("[PASS] kmalloc boundaries\n");
}

void test_kmalloc_8(void)
{
    void *ptr = kmalloc(8);
    assert(ptr != NULL);

    kfree(ptr);

    kprintf("[PASS] kmalloc-8\n");
}

void test_kmalloc_64(void)
{
    void *ptr = kmalloc(64);
    assert(ptr != NULL);

    kfree(ptr);

    kprintf("[PASS] kmalloc-64\n");
}

void test_kmalloc_2048(void)
{
    void *ptr = kmalloc(2048);
    assert(ptr != NULL);

    kfree(ptr);

    kprintf("[PASS] kmalloc-2048\n");
}

void test_kmalloc_zero(void)
{
    void *ptr = kmalloc(0);
    assert(ptr == NULL);

    kprintf("[PASS] kmalloc zero\n");
}

void test_kmalloc_write(void)
{
    u8 *ptr = kmalloc(64);
    assert(ptr != NULL);

    for (size_t i = 0; i < 64; i++)
        ptr[i] = (u8) i;

    for (size_t i = 0; i < 64; i++)
        assert(ptr[i] == (u8) i);

    kfree(ptr);

    kprintf("[PASS] kmalloc write\n");
}

void test_kmalloc_reuse(void)
{
    void *first = kmalloc(64);
    assert(first != NULL);

    kfree(first);

    void *second = kmalloc(64);
    assert(second != NULL);

    kfree(second);

    kprintf("[PASS] kmalloc reuse\n");
}

void test_kzalloc(void)
{
    u8 *ptr = kzalloc(64);
    assert(ptr != NULL);

    for (size_t i = 0; i < 64; i++)
        assert(ptr[i] == 0);

    kfree(ptr);

    kprintf("[PASS] kzalloc\n");
}

void test_ksize_null(void)
{
    assert(ksize(NULL) == 0);

    kprintf("[PASS] ksize null\n");
}

void test_kmalloc_alignment(void)
{
    uintptr_t ptr = (uintptr_t) kmalloc(64);
    assert(ptr != 0);
    assert(ptr % 64 == 0);

    kfree((void *) ptr);

    kprintf("[PASS] kmalloc alignment\n");
}

void test_kzalloc_zero(void)
{
    assert(kzalloc(0) == NULL);

    kprintf("[PASS] kzalloc zero\n");
}

void test_kmalloc_large(void)
{
    size_t size = 4096;
    void *ptr = kmalloc(size);
    assert(ptr != NULL);

    assert(ksize(ptr) >= size);

    memset(ptr, 0xAA, size);
    assert(((u8 *) ptr)[0] == 0xAA);
    assert(((u8 *) ptr)[size - 1] == 0xAA);

    kfree(ptr);

    kprintf("[PASS] kmalloc large\n");
}

void test_custom_slab_lifecycle(void)
{
    struct slab_cache *cache = slab_cache_create("test-cache", 48, 8);
    assert(cache != NULL);
    assert(cache->obj_size >= 48);

    void *p1 = slab_alloc(cache);
    assert(p1 != NULL);
    assert(cache->total_slabs == 1);

    slab_free(cache, p1);
    assert(cache->active_objs == 0);

    slab_cache_shrink(cache);
    assert(cache->total_slabs == 0);

    slab_cache_destroy(cache);

    kprintf("[PASS] custom slab lifecycle\n");
}

void test_kfree_null(void)
{
    // This should just return early without panicking
    kfree(NULL);

    kprintf("[PASS] kfree null\n");
}

void test_ksize_accuracy(void)
{
    void *ptr = kmalloc(17);
    assert(ksize(ptr) == 32);
    kfree(ptr);

    kprintf("[PASS] ksize accuracy\n");
}

void test_kmalloc_slab_exhaustion(void)
{
// 150 allocations of 64 bytes = 9,600 bytes.
// This strictly requires more than two 4KB pages, guaranteeing
// that slabs move from 'partial' to 'full' and new slabs are allocated.
#define NUM_OBJS 150
    void *ptrs[NUM_OBJS];

    // 1. Fill multiple slabs
    for (int i = 0; i < NUM_OBJS; i++) {
        ptrs[i] = kmalloc(64);
        assert(ptrs[i] != NULL);

        // Write to the memory to ensure it's physically backed and doesn't page fault
        memset(ptrs[i], 0xAB, 64);
    }

    // 2. Free them all (forces slabs back to 'partial' and 'free' lists)
    for (int i = 0; i < NUM_OBJS; i++) {
        // Verify memory wasn't corrupted before freeing
        assert(((u8 *) ptrs[i])[0] == 0xAB);
        kfree(ptrs[i]);
    }

    // 3. Re-allocate one object to ensure the free lists are intact and reusable
    void *test_ptr = kmalloc(64);
    assert(test_ptr != NULL);
    kfree(test_ptr);

    kprintf("[PASS] kmalloc slab exhaustion\n");
}

void run_heap_tests()
{
    kprintf("--- Running Heap Unit Tests ---\n");

    kmalloc_init();

    test_kmalloc_boundaries();
    test_kmalloc_8();
    test_kmalloc_64();
    test_kmalloc_2048();
    test_kmalloc_zero();
    test_kmalloc_write();
    test_kmalloc_reuse();
    test_kzalloc();
    test_ksize_null();
    test_kmalloc_alignment();
    test_kzalloc_zero();
    test_kmalloc_large();
    test_custom_slab_lifecycle();
    test_kfree_null();
    test_ksize_accuracy();
    test_kmalloc_slab_exhaustion();

    kprintf("--- All Heap Unit Tests Passed! ---\n");
}
