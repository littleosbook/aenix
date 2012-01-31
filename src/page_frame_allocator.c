#include "page_frame_allocator.h"
#include "log.h"
#include "string.h"
#include "common.h"

#define FOUR_KB     0x1000
#define ONE_MB      0x100000
#define FOUR_MB     0x400000
#define EIGHT_MB    0x800000

#define KERNEL_AREA_PHYSICAL_END    FOUR_MB
#define FS_PHYSICAL_END             EIGHT_MB
#define STACK_MIN_LEN               FOUR_KB

#define MAX_NUM_MEMORY_MAP  100
#define MEMORY_MAP_MAPPED_FLAG  0x01

struct memory_map {
    uint32_t addr;
    uint32_t len;
    uint8_t flags;
};
typedef struct memory_map memory_map_t;

struct page_frame_bitmap {
    uint32_t *start;
    uint32_t len; /* in bits */
};
typedef struct page_frame_bitmap page_frame_bitmap_t;

static page_frame_bitmap_t page_frames;
static memory_map_t mmap[MAX_NUM_MEMORY_MAP];

static uint32_t fill_memory_map(const multiboot_info_t *mbinfo,
                                kernel_meminfo_t *mem)
{
    uint32_t addr, len, i = 0;
    if ((mbinfo->flags & 0x00000020) == 0) {
        return 0;
    }
    multiboot_memory_map_t *entry =
        (multiboot_memory_map_t *) mbinfo->mmap_addr;
    while ((uint32_t) entry < mbinfo->mmap_addr + mbinfo->mmap_length) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            addr = (uint32_t) entry->addr;
            len = (uint32_t) entry->len;
            if (addr <= mem->kernel_physical_start
                    && (addr + len) >= mem->kernel_physical_start) {
                if (addr < mem->kernel_physical_start) {
                    /* The unreserved memory below 1MB is free to use */
                    mmap[i].addr = addr;
                    mmap[i].len = mem->kernel_physical_start - addr;
                    mmap[i].flags |= MEMORY_MAP_MAPPED_FLAG;
                    ++i;
                }

                /* Remove the kernel from free memory */
                mmap[i].addr = mem->kernel_physical_end;
                mmap[i].len = KERNEL_AREA_PHYSICAL_END
                                    - mem->kernel_physical_end - STACK_MIN_LEN;
                mmap[i].flags |= MEMORY_MAP_MAPPED_FLAG;
                ++i;

                /* Remove the fs from free memory */
                mmap[i].addr = FS_PHYSICAL_END;
                mmap[i].len = len - FS_PHYSICAL_END;
                ++i;
            } else {
                mmap[i].addr = addr;
                mmap[i].len = len;
                if (addr + len <= KERNEL_AREA_PHYSICAL_END) {
                    mmap[i].flags |= MEMORY_MAP_MAPPED_FLAG;
                }
                ++i;
            }
        }
        entry = (multiboot_memory_map_t *)
            (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    return i;
}

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

static void construct_bitmap(memory_map_t *mmap, uint32_t n)
{
    uint32_t i, bitmap_size;

    /* calculate number of available page frames */
    for (i = 0; i < n; ++i) {
        page_frames.len += mmap[i].len / FOUR_KB;
    }

    bitmap_size = page_frames.len / 8;

    for (i = 0; i < n; ++i) {
        if (mmap[i].flags & MEMORY_MAP_MAPPED_FLAG) {
            if (mmap[i].len >= bitmap_size) {
                page_frames.start =
                    (uint32_t *) PHYSICAL_TO_VIRTUAL(mmap[i].addr);

                mmap[i].addr += bitmap_size;
                mmap[i].len -= bitmap_size;
                break;
            }
        }
    }

    if (i == n) {
        log_printf("ERROR: pfa: couldn't find place for bitmap; size: %u\n",
                   bitmap_size);
        return;
    }

    log_printf("pfa: bitmap: [start: %X, len: %u]\n",
               page_frames.start, page_frames.len);

    /* mark all memory in mmap as free */
    memset(page_frames.start, 0xFF, page_frames.len/8);
}

void pfa_init(const multiboot_info_t *mbinfo, kernel_meminfo_t *mem)
{
    uint32_t i, n, addr, len;

    n = fill_memory_map(mbinfo, mem);

    for (i = 0; i < n; ++i) {
        log_printf("pfa_init: free mem: [addr: %X, len: %u] conf: %u\n",
                mmap[i].addr, mmap[i].len, mmap[i].flags);

        /* align addresses on 4kB blocks */
        addr = align_up(mmap[i].addr, FOUR_KB);
        len = align_down(mmap[i].len - (addr - mmap[i].addr), FOUR_KB);

        mmap[i].addr = addr;
        mmap[i].len = len;

        log_printf("pfa_init: free mem aligned: [addr: %X, len: %u] conf: %u\n",
                mmap[i].addr, mmap[i].len, mmap[i].flags);
    }

    construct_bitmap(mmap, n);
}
