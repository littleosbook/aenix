#include "page_frame_allocator.h"
#include "log.h"
#include "string.h"
#include "common.h"
#include "constants.h"
#include "mem.h"
#include "paging.h"
#include "math.h"

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

static uint32_t fill_memory_map(multiboot_info_t const *mbinfo,
                                kernel_meminfo_t const *mem,
                                uint32_t fs_paddr, uint32_t fs_size)
{
    uint32_t addr, len, i = 0;
    if ((mbinfo->flags & 0x00000020) == 0) {
        log_error("fill_memory_map", "No memory map from GRUB\n");
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

                addr = mem->kernel_physical_end;
                len = len - mem->kernel_physical_end;

            }

            if (addr > ONE_MB) {
                if (addr < fs_paddr && ((addr + len) > (fs_paddr + fs_size))) {
                    mmap[i].addr = addr;
                    mmap[i].len = fs_paddr - addr;
                    ++i;

                    addr = fs_paddr + fs_size;
                    len -= (fs_paddr + fs_size) - addr;
                }

                mmap[i].addr = addr;
                mmap[i].len = len;
                ++i;
            }
        }
        entry = (multiboot_memory_map_t *)
            (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    return i;
}

static uint32_t construct_bitmap(memory_map_t *mmap, uint32_t n)
{
    uint32_t i, bitmap_pfs, bitmap_size, paddr, vaddr, mapped_mem;
    uint32_t total_pfs = 0;

    /* calculate number of available page frames */
    for (i = 0; i < n; ++i) {
        total_pfs += mmap[i].len / FOUR_KB;
    }

    bitmap_pfs = div_ceil(div_ceil(total_pfs, 8), FOUR_KB);

    for (i = 0; i < n; ++i) {
        if (mmap[i].len >= bitmap_pfs * FOUR_KB) {
            paddr = mmap[i].addr;

            mmap[i].addr += bitmap_pfs * FOUR_KB;
            mmap[i].len -= bitmap_pfs * FOUR_KB;
            break;
        }
    }

    page_frames.len = total_pfs - bitmap_pfs;
    bitmap_size = div_ceil(page_frames.len, 8);

    if (i == n) {
        log_error("construct_bitmap",
                  "Couldn't find place for bitmap. bitmap_size: %u\n",
                   bitmap_size);
        return 1;
    }

    vaddr = pdt_kernel_find_next_vaddr(bitmap_size);
    if (vaddr == 0) {
        log_error("construct_bitmap",
                  "Could not find virtual address for bitmap in kernel. "
                  "paddr: %X, bitmap_size: %u, bitmap_pfs: %u\n",
                  paddr, bitmap_size);
        return 1;

    }
    log_info("construct_bitmap",
             "bitmap vaddr: %X, bitmap paddr: %X, page_frames.len: %u, "
             "bitmap_size: %u, bitmap_pfs: %u\n",
              vaddr, paddr, page_frames.len, bitmap_size, bitmap_pfs);

    mapped_mem = pdt_map_kernel_memory(paddr, vaddr, bitmap_size,
                                       PAGING_PL0, PAGING_READ_WRITE);
    if (mapped_mem < bitmap_size) {
        log_error("construct_bitmap",
                  "Could not map kernel memory for bitmap. "
                  "paddr: %X, vaddr: %X, bitmap_size: %u\n",
                  paddr, vaddr, bitmap_size);
        return 1;
    }

    page_frames.start = (uint32_t *) vaddr;

    memset(page_frames.start, 0xFF, bitmap_size);
    uint8_t *last = (uint8_t *)((uint32_t)page_frames.start + bitmap_size - 1);
    *last = 0;
    for (i = 0; i < page_frames.len % 8; ++i) {
        *last |= 0x01 << (7 - i);
    }

    return 0;
}

uint32_t pfa_init(multiboot_info_t const *mbinfo,
              kernel_meminfo_t const *mem,
              uint32_t fs_paddr, uint32_t fs_size)
{
    uint32_t i, n, addr, len;

    n = fill_memory_map(mbinfo, mem, fs_paddr, fs_size);
    if (n == 0) {
        return 1;
    }

    log_info("pfa_init",
              "\n\tkernel_physical_start: %X\n"
              "\tkernel_physical_end: %X\n"
              "\tkernel_virtual_start: %X\n"
              "\tkernel_virtual_end: %X\n",
              mem->kernel_physical_start, mem->kernel_physical_end,
              mem->kernel_virtual_start, mem->kernel_virtual_end);

    mmap_len = n;

    for (i = 0; i < n; ++i) {
        /* align addresses on 4kB blocks */
        addr = align_up(mmap[i].addr, FOUR_KB);
        len = align_down(mmap[i].len - (addr - mmap[i].addr), FOUR_KB);

        mmap[i].addr = addr;
        mmap[i].len = len;

        log_debug("pfa_init", "mmap[%u] -> addr: %X, len: %u, pfs: %u\n", i, addr, len, len / FOUR_KB);
    }

    return construct_bitmap(mmap, n);
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
        if (current_offset + mmap[i].len <= offset) {
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

static uint32_t fits_in_one_mmap_entry(uint32_t bit_idx, uint32_t pfs)
{
    uint32_t i, current_offset = 0, offset = bit_idx * FOUR_KB;
    for (i = 0; i < mmap_len; ++i) {
        if (current_offset + mmap[i].len <= offset) {
            current_offset += mmap[i].len;
        } else {
            offset -= current_offset;
            if (offset + pfs * FOUR_KB <= mmap[i].len) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    return 0;
}

uint32_t pfa_allocate(uint32_t num_page_frames)
{
    uint32_t i, j, cell, bit_idx;
    uint32_t n = div_ceil(page_frames.len, 32), frames_found = 0;

    for (i = 0; i < n; ++i) {
        cell = page_frames.start[i];
        if (cell != 0) {
            for (j = 0; j < 32; ++j) {
                if (((cell >> (31 - j)) & 0x1) == 1) {
                    if (frames_found == 0) {
                        bit_idx = i * 32 + j;
                    }
                    ++frames_found;
                    if (frames_found == num_page_frames) {
                        if (fits_in_one_mmap_entry(bit_idx, num_page_frames)) {
                            toggle_bits(bit_idx, num_page_frames);
                            return paddr_for_idx(bit_idx);
                        } else {
                            frames_found = 0;
                        }
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
        log_error("pfa_free", "invalid paddr %X\n", paddr);
    } else {
        toggle_bit(bit_idx);
    }
}

void pfa_free_cont(uint32_t paddr, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i) {
        pfa_free(paddr + i * FOUR_KB);
    }
}
