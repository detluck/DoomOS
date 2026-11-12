#pragma once

#include "types.h"
#include "lib/list.h"

#define KMALLOC_MIN_SIZE 8
#define KMALLOC_MAX_SIZE 2048
#define KMALLOC_NUM_CACHES 9

#define LARGE_MAGIC 0x4C415247454D4147
#define SLAB_MAGIC 0x534C41424D414749

// kernel heap

void kmalloc_init(void);

// allocate kernel memory
void *kmalloc(size_t size);

// allocate zeroed kernel memory
void *kzalloc(size_t size);

// free kernel memory
void kfree(void *ptr);

// get allocation size
size_t ksize(void *ptr);

// heap statistics
void kheap_dump_stats(void);

// slab allocator

struct slab_cache {
    const char *name;    // cache name for debugging
    size_t obj_size;     // object size
    size_t align;        // object alignment
    size_t obj_per_slab; // objects per slab

    struct list_head slabs_full;    // full slabs
    struct list_head slabs_partial; // partially full slabs
    struct list_head slabs_free;    // empty slabs

    u64 total_allocs; // total allocations
    u64 total_frees;  // total frees
    u64 active_objs;  // currently allocated objects
    u64 total_slabs;  // total slab count

    // spinlock_t lock;   as soon as interrupts come
    struct list_head link; // link in global cache list
};

struct slab_cache *slab_cache_create(const char *name, size_t size, size_t align);

// destroy a slab cache
void slab_cache_destroy(struct slab_cache *cache);

// allocate from a slab cache
void *slab_alloc(struct slab_cache *cache);

// free to a slab cache
void slab_free(struct slab_cache *cache, void *obj);

// shrink a slab cache (free empty slabs)
void slab_cache_shrink(struct slab_cache *cache);

// dump slab cache statistics
void slab_cache_dump(struct slab_cache *cache);
