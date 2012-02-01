#include "page_frame_allocator.h"
#include "log.h"
#include "string.h"
#include "common.h"
#include "constants.h"
#include "mem.h"
#include "paging.h"

#define MAX_NUM_MEMORY_MAP  100

struct memory_map {
    uint32_t addr;
    uint32_t len;
};
typedef struct memory_map memory_map_t;

struct page_frame_bitmap {
    uint32_t *start;
    uint32_t len; /* in bits */
};
typedef struct page_frame_bitmap page_frame_bitmap_t;

static page_frame_bitmap_t page_frames;
static memory_map_t mmap[MAX_NUM_MEMORY_MAP];
static uint32_t mmap_len;

static uint32_t fill_memory_map(const multiboot_info_t *mbinfo,
                                kernel_meminfo_t *mem);
static void construct_bitmap(memory_map_t *mmap, uint32_t n);

void pfa_init(const multiboot_info_t *mbinfo, kernel_meminfo_t *mem)
{
    uint32_t i, n, addr, len;

    n = fill_memory_map(mbinfo, mem);
    mmap_len = n;

    for (i = 0; i < n; ++i) {
        log_printf("pfa_init: free mem: [addr: %X, len: %u]\n",
                mmap[i].addr, mmap[i].len);

        /* align addresses on 4kB blocks */
        addr = align_up(mmap[i].addr, FOUR_KB);
        len = align_down(mmap[i].len - (addr - mmap[i].addr), FOUR_KB);

        mmap[i].addr = addr;
        mmap[i].len = len;

        log_printf("pfa_init: free mem aligned: [addr: %X, len: %u]\n",
                mmap[i].addr, mmap[i].len);
    }

    construct_bitmap(mmap, n);
}

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
                    && (addr + len) > mem->kernel_physical_end) {

                mmap[i].addr = mem->kernel_physical_end;
                mmap[i].len = len - mem->kernel_physical_end;
                ++i;

            } else if (addr > ONE_MB) {
                mmap[i].addr = addr;
                mmap[i].len = len;
                ++i;
            }
        }
        /* TODO: WATCH OUT FOR MODULES! */
        entry = (multiboot_memory_map_t *)
            (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    return i;
}

static uint32_t ceil(uint32_t num, uint32_t den)
{
    return (num - 1) / den + 1;
}

static void construct_bitmap(memory_map_t *mmap, uint32_t n)
{
    uint32_t i, bitmap_size, physical_addr, virtual_addr;
    uint32_t *last;

    /* calculate number of available page frames */
    for (i = 0; i < n; ++i) {
        page_frames.len += mmap[i].len / FOUR_KB;
    }

    bitmap_size = ceil(page_frames.len, 8);

    for (i = 0; i < n; ++i) {
        if (mmap[i].len >= bitmap_size) {
            physical_addr = mmap[i].addr;

            mmap[i].addr += bitmap_size;
            mmap[i].len -= bitmap_size;
            break;
        }
    }

    if (i == n) {
        log_printf("ERROR: pfa: couldn't find place for bitmap; size: %u\n",
                   bitmap_size);
        return;
    }

    virtual_addr = pdt_kernel_find_next_vaddr(bitmap_size);
    log_printf("pfa: bitmap: [start: %X, size: %u page frames, %u bytes]\n",
               virtual_addr, page_frames.len, bitmap_size);

    pdt_map_kernel_memory(physical_addr, virtual_addr, bitmap_size,
                          PAGING_PL0, PAGING_READ_WRITE);

    page_frames.start = (uint32_t *) virtual_addr;


    memset(page_frames.start, 0xFF, bitmap_size);
    last = page_frames.start + bitmap_size - 1;
    *last = 0;
    for (i = 0; i < page_frames.len % 8; ++i) {
        *last |= 0x01 << (7 - i);
    }
}

static void toggle_bit(uint32_t bit_idx)
{
    uint32_t *bits = page_frames.start;
    bits[bit_idx/32] ^= (0x01 << (31 - (bit_idx % 32)));
}

static void toggle_bits(uint32_t bit_idx, uint32_t num_bits)
{
    uint32_t i;
    for (i = bit_idx; i < bit_idx + num_bits; ++i) {
        toggle_bit(i);
    }
}

static uint32_t paddr_for_idx(uint32_t bit_idx)
{
    uint32_t i, current_offset = 0, offset = bit_idx * FOUR_KB;
    for (i = 0; i < mmap_len; ++i) {
        if (current_offset + mmap[i].len < offset) {
            current_offset += mmap[i].len;
        } else {
            offset -= current_offset;
            return mmap[i].addr + offset;
        }
    }

    return 0;
}

static uint32_t idx_for_paddr(uint32_t paddr)
{
    uint32_t i, byte_offset = 0;
    for (i = 0; i < mmap_len; ++i) {
        if (paddr < mmap[i].addr + mmap[i].len) {
            byte_offset += paddr - mmap[i].addr;
            return byte_offset / FOUR_KB;
        } else {
            byte_offset += mmap[i].len;
        }
    }

    return page_frames.len;
}

uint32_t pfa_allocate(uint32_t num_page_frames)
{
    uint32_t i, j, cell, bit_idx;
    uint32_t n = ceil(page_frames.len, 32), frames_found = 0;

    for (i = 0; i < n; ++i) {
        cell = page_frames.start[i];
        if (cell != 0) {
            for (j = 0; j < 32; ++j) {
                if (((cell >> (31 - j)) & 0x1) == 1) {
                    ++frames_found;
                    if (frames_found == num_page_frames) {
                        bit_idx = i * 32 + j - num_page_frames;
                        toggle_bits(bit_idx, num_page_frames);
                        return paddr_for_idx(bit_idx);
                    }
                } else {
                    frames_found = 0;
                }
            }
        } else {
            frames_found = 0;
        }
    }

    return 0;
}

void pfa_free(uint32_t paddr)
{
    uint32_t bit_idx = idx_for_paddr(paddr);
    if (bit_idx == page_frames.len) {
        log_printf("ERROR: pfa_free: invalid paddr %X\n", paddr);
    } else {
        toggle_bit(bit_idx);
    }
}
