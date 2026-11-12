#include "mm/heap.h"
#include "defs.h"
#include "mm/pmm.h"
#include "arch/boot.h"
#include "lib/list.h"
#include "lib/string.h"
#include "lib/kprintf.h"
#include "lib/assert.h"
#include "types.h"

struct slab {
    u64 magic;
    struct slab_cache *cache;
    struct list_head link;
    void *free;
    u32 in_use;
};

// header for large allocation, avoiding slabs use allocator direct
struct large_hdr {
    u64 magic;
    size_t num_pages;
};

// global cachlist
static LIST_HEAD(caches);
// cache classes from 8 to 2048
static struct slab_cache *kmalloc_caches[KMALLOC_NUM_CACHES];
static bool kmalloc_initialized = false;

static void *page_alloc(void)
{
    phys_addr_t phys = pmm_alloc_frame();
    if (!phys) {
        return NULL;
        ;
        ;
    }
    return (void *) PHYS_TO_VIRT_DIRECT(phys);
}

static void page_free(void *page)
{
    pmm_free_frame(VIRT_TO_PHYS_DIRECT(page));
}

static u32 get_obj_per_slab(size_t obj_size, size_t align)
{
    // if (obj_size < sizeof(void *)) {
    //     obj_size = sizeof(void *);
    // }
    obj_size = ALIGN_UP(obj_size, align);

    size_t first = ALIGN_UP(sizeof(struct slab), align);
    if (first >= PAGE_SIZE) {
        return 0;
    }
    return (PAGE_SIZE - first) / obj_size;
}

static struct slab *obj_to_slab(void *obj)
{
    return (struct slab *) ALIGN_DOWN((u64) obj, PAGE_SIZE);
}

static size_t get_free_obj(struct slab_cache *cache, struct slab *slab)
{
    return cache->obj_per_slab - slab->in_use;
}

static bool is_large_page_start(const void *ptr, const struct large_hdr *h)
{
    return (const u8 *) ptr == (const u8 *) h + sizeof(struct large_hdr);
}

static u64 *get_page(void *ptr)
{

    return (u64 *) ALIGN_DOWN((virt_addr_t) ptr, PAGE_SIZE);
}

static struct slab_cache *get_cache_for_size(size_t size)
{

    if (size <= 8)
        return kmalloc_caches[0];
    if (size <= 16)
        return kmalloc_caches[1];
    if (size <= 32)
        return kmalloc_caches[2];
    if (size <= 64)
        return kmalloc_caches[3];
    if (size <= 128)
        return kmalloc_caches[4];
    if (size <= 256)
        return kmalloc_caches[5];
    if (size <= 512)
        return kmalloc_caches[6];
    if (size <= 1024)
        return kmalloc_caches[7];
    if (size <= 2048)
        return kmalloc_caches[8];

    return NULL;
}

static bool slab_ptr_is_slot(struct slab *s, void *ptr)
{
    if (s->magic != SLAB_MAGIC || !s->cache)
        return false;

    struct slab_cache *c = s->cache;

    uintptr_t first = (uintptr_t) s + ALIGN_UP(sizeof(struct slab), c->align);

    uintptr_t p = (uintptr_t) ptr;
    uintptr_t offset = p - first;

    if (p < first || p >= first + c->obj_per_slab * c->obj_size)
        return false;

    return offset % c->obj_size == 0;
}

static void slab_free_slab(struct slab *slab)
{

    list_del(&slab->link);
    slab->magic = 0;
    page_free(slab);
}

static struct slab *slab_alloc_new(struct slab_cache *cache)
{
    struct slab *s = page_alloc();
    if (!s) {
        return NULL;
    }
    s->magic = SLAB_MAGIC;
    s->cache = cache;
    INIT_LIST_HEAD(&s->link);
    s->in_use = 0;

    void *first = (void *) ((uintptr_t) s + ALIGN_UP(sizeof(struct slab), cache->align));
    for (size_t i = 0; i < cache->obj_per_slab; i++) {
        u8 *slot = first + i * cache->obj_size;
        void *next = i + 1 < cache->obj_per_slab ? first + (i + 1) * cache->obj_size : NULL;
        *(void **) slot = next;
    }
    s->free = first;

    cache->total_slabs++;
    return s;
}

struct slab_cache *slab_cache_create(const char *name, size_t size, size_t align)
{
    if (size == 0) {
        return NULL;
    }
    if (align == 0) {
        align = sizeof(void *);
    }
    if (align & (align - 1)) { // must be a power of two
        return NULL;
    }
    if (size < sizeof(void *)) {
        size = sizeof(void *);
    }
    size = ALIGN_UP(size, align);

    u32 per_slab = get_obj_per_slab(size, align);
    if (per_slab == 0) {
        return NULL;
    }

    struct slab_cache *c = page_alloc();
    if (!c) {
        return NULL;
    }
    memset(c, 0, sizeof(*c));

    c->name = name;
    c->obj_size = size;
    c->align = align;
    c->obj_per_slab = per_slab;

    INIT_LIST_HEAD(&c->slabs_free);
    INIT_LIST_HEAD(&c->slabs_partial);
    INIT_LIST_HEAD(&c->slabs_full);
    list_add(&c->link, &caches);

    return c;
}

void slab_cache_destroy(struct slab_cache *cache)
{
    if (!cache)
        return;

    list_del(&cache->link);

    struct slab *s, *tmp;
    struct list_head *lists[] = {&cache->slabs_full, &cache->slabs_partial, &cache->slabs_free};
    for (int i = 0; i < 3; i++) {
        list_for_each_entry_safe(s, tmp, lists[i], link)
        {
            slab_free_slab(s);
        }
    }

    page_free(cache);
}

void *slab_alloc(struct slab_cache *cache)
{
    if (!cache)
        return NULL;

    struct slab *s = NULL;

    // try partial list first
    if (!list_empty(&cache->slabs_partial)) {
        s = list_first_entry(&cache->slabs_partial, struct slab, link);
    } else if (!list_empty(&cache->slabs_free)) {
        s = list_first_entry(&cache->slabs_free, struct slab, link);
        list_move(&s->link, &cache->slabs_partial);
    } else {
        s = slab_alloc_new(cache);
        if (!s)
            return NULL;
        list_add(&s->link, &cache->slabs_partial);
    }

    // get free object
    void *obj = NULL;
    obj = s->free;
    s->free = *(void **) obj;
    s->in_use++;

    // check if there are free slots
    if (cache->obj_per_slab == s->in_use) {
        list_move(&s->link, &cache->slabs_full);
    }

    cache->total_allocs++;
    cache->active_objs++;

    return obj;
}

void slab_free(struct slab_cache *cache, void *obj)
{
    if (!cache || !obj) {
        return;
    }

    struct slab *s = obj_to_slab(obj);

    if (!slab_ptr_is_slot(s, obj)) {
        panic("slab_free: invalid pointer");
    }
    if (s->in_use == 0) {
        panic("slab_free: double free or corrupted slab");
    }
    // update freelist
    *(void **) obj = s->free;
    s->free = obj;
    s->in_use--;

    // check if slab empty now
    if (s->in_use == 0) {
        list_move(&s->link, &cache->slabs_free);
    } else if (get_free_obj(cache, s) == 1) {
        list_move(&s->link, &cache->slabs_partial);
    }

    cache->total_frees++;
    cache->active_objs--;
}

void slab_cache_shrink(struct slab_cache *cache)
{
    if (!cache) {
        return;
    }
    struct slab *s, *tmp;
    list_for_each_entry_safe(s, tmp, &cache->slabs_free, link)
    {
        s->cache->total_slabs--;
        slab_free_slab(s);
    }
}

void slab_cache_dump(struct slab_cache *cache)
{
    if (!cache) {
        return;
    }

    kprintf("Name: %s\n", cache->name);
    kprintf("Object size: %lu\n", cache->obj_size);
    kprintf("Align: %lu\n", cache->align);
    kprintf("Objects per slab: %lu\n", cache->obj_per_slab);
    kprintf("Total allocations: %lu\n", cache->total_allocs);
    kprintf("Total frees: %lu\n", cache->total_frees);
    kprintf("Total slabs: %lu\n", cache->total_slabs);
}

void kmalloc_init(void)
{
    if (kmalloc_initialized) {
        return;
    }

    static const char *names[] = {"kmalloc-8",   "kmalloc-16",   "kmalloc-32",
                                  "kmalloc-64",  "kmalloc-128",  "kmalloc-256",
                                  "kmalloc-512", "kmalloc-1024", "kmalloc-2048"};

    size_t size = KMALLOC_MIN_SIZE;
    for (size_t i = 0; i < KMALLOC_NUM_CACHES; i++) {

        kmalloc_caches[i] = slab_cache_create(names[i], size, size);
        if (!kmalloc_caches[i]) {
            panic("kmalloc_init: failed to create cache");
        }
        size *= 2;
    }
    kmalloc_initialized = true;
}

void *kmalloc(size_t size)
{
    assert(kmalloc_initialized);
    if (size == 0) {
        return NULL;
    }

    // allocate directly
    if (size > KMALLOC_MAX_SIZE) {
        if (size > SIZE_MAX - sizeof(struct large_hdr) - PAGE_SIZE) {
            return NULL;
        }

        u64 count = ALIGN_UP(size + sizeof(struct large_hdr), PAGE_SIZE) / PAGE_SIZE;

        phys_addr_t p = pmm_alloc_frames(count);
        if (!p) {
            return NULL;
        }
        struct large_hdr *h = (struct large_hdr *) PHYS_TO_VIRT_DIRECT(p);
        h->magic = LARGE_MAGIC;
        h->num_pages = count;
        return (u8 *) h + sizeof(*h);
    }
    struct slab_cache *c = get_cache_for_size(size);
    if (!c) {
        return NULL;
    }
    return slab_alloc(c);
}

void *kzalloc(size_t size)
{
    void *p = kmalloc(size);
    if (!p) {
        return NULL;
    }

    memset(p, 0, size);
    return p;
}

void kfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    u64 *p = get_page(ptr);
    // read magic
    if (*p == LARGE_MAGIC) {
        struct large_hdr *h = (struct large_hdr *) p;
        if (!is_large_page_start(ptr, h)) {
            panic("kfree: pointer is not the start of a large allocation");
        }
        u64 pages = h->num_pages;
        h->magic = 0;
        pmm_free_frames(VIRT_TO_PHYS_DIRECT(p), pages);
    } else if (*p == SLAB_MAGIC) {
        struct slab *s = (struct slab *) p;
        slab_free(s->cache, ptr);
    } else {
        panic("kfree: trying to free invalid pointer");
    }
}

size_t ksize(void *ptr)
{
    if (!ptr) {
        return 0;
    }
    u64 *p = get_page(ptr);
    if (*p == LARGE_MAGIC) {
        struct large_hdr *h = (struct large_hdr *) p;
        if (!is_large_page_start(ptr, h)) {
            panic("ksize: pointer is not the start of a large allocation");
        }
        return h->num_pages * PAGE_SIZE - sizeof(struct large_hdr);
    } else if (*p == SLAB_MAGIC) {
        struct slab *s = (struct slab *) p;
        return s->cache->obj_size;
    }
    panic("ksize: invalid pointer");
}

void kheap_dump_stats(void)
{
    u64 total_active = 0;
    u64 total_slabs = 0;

    for (int i = 0; i < KMALLOC_NUM_CACHES; i++) {
        if (kmalloc_caches[i]) {
            struct slab_cache *cache = kmalloc_caches[i];
            kprintf("  %s: %lu active, %lu slabs\n", cache->name, cache->active_objs,
                    cache->total_slabs);
            total_active += cache->active_objs;
            total_slabs += cache->total_slabs;
        }
    }

    kprintf("  Total: %lu active objects, %lu slabs (%lu KiB)\n", total_active, total_slabs,
            (total_slabs * PAGE_SIZE) / 1024);
}
