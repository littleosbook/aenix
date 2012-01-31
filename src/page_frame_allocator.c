#include "page_frame_allocator.h"
#include "log.h"

#define FOUR_KB     0x1000

struct page_frame {
    uint32_t size; /* in 4kB blocks */
    struct page_frame *next;
};
typedef struct page_frame page_frame_t;

static uint32_t align_up(uint32_t n, uint32_t a)
{
    uint32_t m = n % a;
    if (m == 0) {
        return n;
    }
    return n + (a - m);
}

static uint32_t align_down(uint32_t n, uint32_t a)
{
    return n - (n % a);
}

void pfa_init(memory_map_t *mmap, uint32_t n)
{
    uint32_t i, addr, len;

    for (i = 0; i < n; ++i) {
        log_printf("pfa_init: free mem: [addr: %X, len: %u] conf: %u\n",
                  mmap[i].addr, mmap[i].len, mmap[i].flags);
        /* align addresses on 4kB blocks */
        addr = align_up(mmap[i].addr, FOUR_KB);
        len = mmap[i].len - (addr - mmap[i].addr);

        mmap[i].addr = addr;
        mmap[i].len = align_down(len, FOUR_KB);

        log_printf("pfa_init: free mem aligned: [addr: %X, len: %u] conf: %u\n",
                  mmap[i].addr, mmap[i].len, mmap[i].flags);
    }
}
