#include "string.h"
#include "stdint.h"
#include "stdio.h"
#include "common.h"
#include "log.h"
#include "kmalloc.h"
#include "log.h"
#include "mem.h"
#include "constants.h"

#define NUM_ENTRIES 1024
#define PDT_SIZE NUM_ENTRIES * sizeof(pde_t)
#define PT_SIZE  NUM_ENTRIES * sizeof(pte_t)

#define VIRTUAL_TO_PDT_IDX(a)   (((a) >> 22) & 0x3FF)
#define VIRTUAL_TO_PT_IDX(a)    (((a) >> 12) & 0x3FF)
#define PDT_IDX_TO_VIRTUAL(a)   (((a) << 22))
#define PT_IDX_TO_VIRTUAL(a)   (((a) << 12))

#define PS_4KB 0x00
#define PS_4MB 0x01

#define IS_ENTRY_PRESENT(e) ((e)->config && 0x01)
#define IS_ENTRY_PAGE_TABLE(e) (((e)->config && 0x80) == 0)

#define PT_ENTRY_SIZE  FOUR_KB
#define PDT_ENTRY_SIZE FOUR_MB

#define KERNEL_TMP_PT_IDX   1023

/* pde: page directory entry */
struct pde {
    uint8_t config;
    uint8_t low_addr; /* only the highest 4 bits are used */
    uint16_t high_addr;
} __attribute__((packed));

#include "paging.h" /* must be included after the pde struct */

struct pte {
    uint8_t config;
    uint8_t middle; /* only the highest 4 bits and the lowest bit are used */
    uint16_t high_addr;
    } __attribute__((packed));
typedef struct pte pte_t;

static pde_t *kernel_pdt;
static pte_t *kernel_pt;

extern void pdt_set(uint32_t pdt_addr); /* defined in paging_asm.s */
extern void invalidate_page_table_entry(uint32_t virtual_addr);


static void create_pdt_entry(pde_t *pdt,
                             uint32_t n,
                             uint32_t addr,
                             uint8_t ps,
                             uint8_t rw,
                             uint8_t pl);
static void create_pt_entry(pte_t *pt,
                            uint32_t n,
                            uint32_t addr,
                            uint8_t rw,
                            uint8_t pl);

static uint32_t get_pt_paddr(pde_t *pde, uint32_t pde_idx)
{
    pde_t *e = pde + pde_idx;
    uint32_t addr = e->high_addr;
    addr <<= 16;
    addr |= ((uint32_t) (e->low_addr & 0xF0) << 8);

    return addr;
}

static uint32_t kernel_map_temporary_memory(uint32_t paddr)
{
    uint32_t vaddr = KERNEL_START_VADDR + KERNEL_TMP_PT_IDX * PT_ENTRY_SIZE;
    create_pt_entry(kernel_pt, KERNEL_TMP_PT_IDX, paddr,
                    PAGING_READ_WRITE, PAGING_PL0);
    invalidate_page_table_entry(vaddr);
    return vaddr;
}

void paging_init(uint32_t kernel_pdt_vaddr, uint32_t kernel_pt_vaddr)
{
    kernel_pdt = (pde_t *) kernel_pdt_vaddr;
    kernel_pt = (pte_t *) kernel_pt_vaddr;
}

static uint32_t pt_kernel_find_next_virtual_addr(uint32_t pdt_idx,
                                                 pte_t *pt, uint32_t size)
{
    uint32_t i, num_to_find, num_found;
    num_to_find = align_up(size, 4096) / 4096;
    for (i = 0, num_found = 0; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(pt+i)) {
            num_found = 0;
        } else {
            ++num_found;
            if (num_found == num_to_find) {
                return PDT_IDX_TO_VIRTUAL(pdt_idx) |
                       PT_IDX_TO_VIRTUAL(i-num_found);
            }
        }
    }
    return 0;
}

uint32_t pdt_kernel_find_next_virtual_addr(uint32_t size)
{
    uint32_t pdt_idx, pt_paddr, pt_vaddr, virtual_addr = 0;
    /* TODO: support > 4MB sizes */

    pdt_idx = VIRTUAL_TO_PDT_IDX(KERNEL_START_VADDR);
    for (; pdt_idx < NUM_ENTRIES; ++pdt_idx) {
        if (IS_ENTRY_PRESENT(kernel_pdt + pdt_idx)) {
            pt_paddr = get_pt_paddr(kernel_pdt, pdt_idx);
            pt_vaddr = kernel_map_temporary_memory(pt_paddr);
            virtual_addr = pt_kernel_find_next_virtual_addr(pdt_idx,
                               (pte_t *)pt_vaddr, size);
        } else {
            virtual_addr = PDT_IDX_TO_VIRTUAL(pdt_idx);
        }
        if (virtual_addr != 0) {
            break;
        }
    }
    return virtual_addr;
}

static uint32_t pt_map_memory(pte_t *pt,
                              uint32_t physical_addr,
                              uint32_t virtual_addr,
                              uint32_t size,
                              uint8_t rw,
                              uint8_t pl)
{
    uint32_t pt_idx = VIRTUAL_TO_PT_IDX(virtual_addr);
    uint32_t mapped_size = 0;

    while (mapped_size < size && pt_idx < NUM_ENTRIES) {
        if (IS_ENTRY_PRESENT(pt + pt_idx)) {
            return 0;
        }
        create_pt_entry(pt, pt_idx, physical_addr, rw, pl);

        physical_addr += PT_ENTRY_SIZE;
        mapped_size += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return mapped_size;
}

uint32_t pdt_map_memory(pde_t *pdt,
                        uint32_t physical_addr,
                        uint32_t virtual_addr,
                        uint32_t size,
                        uint8_t rw,
                        uint8_t pl)
{
    uint32_t pdt_idx;
    pte_t *pt;
    uint32_t pt_paddr, pt_vaddr;
    uint32_t mapped_size = 0;
    uint32_t total_mapped_size = 0;
    size = align_up(size, PT_ENTRY_SIZE);

    while (size != 0) {
        pdt_idx = VIRTUAL_TO_PDT_IDX(virtual_addr);

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            /* FIXME: */
            pt = kmalloc(PT_SIZE);
            if (pt == NULL) {
                return 0;
            }
            memset(pt, PT_SIZE, 0);
        } else {
            pt_paddr = get_pt_paddr(pdt, pdt_idx);
            pt_vaddr = kernel_map_temporary_memory(pt_paddr);
            pt = (pte_t *) pt_vaddr;
        }

        mapped_size =
            pt_map_memory(pt, physical_addr, virtual_addr, size, rw, pl);
        if (mapped_size == 0) {
            return 0;
        }

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            create_pdt_entry(pdt, pdt_idx, (uint32_t) pt, PS_4KB, rw, pl);
        }

        size -= mapped_size;
        total_mapped_size += mapped_size;
        virtual_addr += mapped_size;
        physical_addr += mapped_size;
    }

    return total_mapped_size;
}

uint32_t pdt_map_kernel_memory(uint32_t physical_addr,
                               uint32_t virtual_addr,
                               uint32_t size,
                               uint8_t rw,
                               uint8_t pl)
{
    return pdt_map_memory(kernel_pdt, physical_addr, virtual_addr,
                          size, rw, pl);
}

static uint32_t pt_unmap_memory(pte_t *pt,
                                uint32_t virtual_addr,
                                uint32_t size)
{
    uint32_t pt_idx = VIRTUAL_TO_PT_IDX(virtual_addr);
    uint32_t freed_size = 0;

    while (freed_size < size && pt_idx < NUM_ENTRIES) {
        if (IS_ENTRY_PRESENT(pt + pt_idx)) {
            memset(pt + pt_idx, 0, sizeof(pte_t));
            invalidate_page_table_entry(virtual_addr);
        }

        freed_size += PT_ENTRY_SIZE;
        virtual_addr += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return freed_size;
}

uint32_t pdt_unmap_memory(pde_t *pdt, uint32_t virtual_addr, uint32_t size)
{
    uint32_t pdt_idx;
    pte_t *pt;
    uint32_t freed_size = 0;
    uint32_t end_virtual_addr;
    size = align_up(size, PT_ENTRY_SIZE);
    end_virtual_addr = virtual_addr + size;

    while (virtual_addr < end_virtual_addr) {
        pdt_idx = VIRTUAL_TO_PDT_IDX(virtual_addr);

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            virtual_addr = align_up(virtual_addr, PDT_ENTRY_SIZE);
            continue;
        }

        pt = (pte_t *) get_pt_paddr(pdt, pdt_idx);
        freed_size =
            pt_unmap_memory(pt, virtual_addr, size);

        if (freed_size == PT_ENTRY_SIZE) {
            kfree(pt);
            memset(pdt + pdt_idx, 0, sizeof(pde_t));
        }

        virtual_addr += freed_size;
    }

    return freed_size;
}

uint32_t pdt_unmap_kernel_memory(uint32_t virtual_addr, uint32_t size)
{
    return pdt_unmap_memory(kernel_pdt, virtual_addr, size);
}

pde_t *pdt_create(void)
{
    pde_t *pdt = kmalloc(PDT_SIZE);
    uint32_t i;

    if (pdt == NULL) {
        return NULL;
    }

    for (i = 0; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(pdt + i)) {
            pdt[i] = kernel_pdt[i];
        }
    }

    return pdt;
}

void pdt_delete(pde_t *pdt)
{
    uint32_t i;
    for (i = 0; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(pdt + i) && IS_ENTRY_PAGE_TABLE(pdt + i)) {
            kfree((pte_t *) get_pt_paddr(pdt, i));
        }
    }

    kfree(pdt);
}

/**
 * Creates an entry in the page descriptor table at the specified index.
 * THe entry will point to the given PTE.
 *
 * @param pdt   The page descriptor table
 * @param n     The index in the PDT
 * @param addr  The address to the first entry in the page table, or a
 *              4MB page frame
 * @param ps    Page size, either PS_4KB or PS_4MB
 * @param rw    Read/write permission, 0 = read-only, 1 = read and write
 * @param pl    The required privilege level to access the page,
 *              0 = PL0, 1 = PL3
 */
static void create_pdt_entry(pde_t *pdt,
                             uint32_t n,
                             uint32_t addr,
                             uint8_t ps,
                             uint8_t rw,
                             uint8_t pl)
{
    /* Since page tables are aligned at 4kB boundaries, we only need to store
     * the 20 highest bits */
    /* The lower 4 bits */
    pdt[n].low_addr  = ((addr >> 12) & 0xF) << 4;
    pdt[n].high_addr = ((addr >> 16) & 0xFFFF);

    /*
     * name    | value | size | desc
     * ---------------------------
     *       P |     1 |    1 | If the entry is present or not
     *     R/W |    rw |    1 | Read/Write:
     *                              0 = Read-only
     *                              1 = Write and Read
     *     U/S |    pl |    1 | User/Supervisor:
     *                              0 = PL3 can't access
     *                              1 = PL3 can access
     *     PWT |     1 |    1 | Page-level write-through:
     *                              0 = writes are cached
     *                              1 = writes are not cached
     *     PCD |     0 |    1 | Page-level cache disable
     *       A |     0 |    1 | Is set if the entry has been accessed
     * Ignored |     0 |    1 | Ignored
     *      PS |    ps |    1 | Page size:
     *                              0 = address point to pt entry,
     *                              1 = address points to 4 MB page
     * Ignored |     0 |    4 | Ignored
     *
     * NOTE: Ignored is not part of pdt[n].config!
     */
    pdt[n].config =
        ((ps & 0x01) << 7) | (0x01 << 3) | ((pl & 0x01) << 2) |
        ((rw & 0x01) << 1) | 0x01;
}

/**
 * Creates a new entry at the specified index in the given page table.
 *
 * @param pt    The page table
 * @param n     The index in the page table to create the entry at
 * @param addr  The addres to the page frame
 * @param rw    Read/write permission, 0 = read-only, 1 = read and write
 * @param pl    The required privilege level to access the page,
 *              0 = PL0, 1 = PL3
 */
static void create_pt_entry(pte_t *pt,
                            uint32_t n,
                            uint32_t addr,
                            uint8_t rw,
                            uint8_t pl)
{
    /* Since page tables are aligned at 4kB boundaries, we only need to store
     * the 20 highest bits */
    /* The lower 4 bits */
    pt[n].middle  = ((addr >> 12) & 0xF) << 4;
    pt[n].high_addr = ((addr >> 16) & 0xFFFF);

    /*
     * name    | value | size | desc
     * ---------------------------
     *       P |     1 |    1 | If the entry is present or not
     *     R/W |    rw |    1 | Read/Write:
     *                              0 = Read-only
     *                              1 = Write and Read
     *     U/S |    pl |    1 | User/Supervisor:
     *                              0 = PL3 can't access
     *                              1 = PL3 can access
     *     PWT |     1 |    1 | Page-level write-through:
     *                              0 = writes are cached
     *                              1 = writes are not cached
     *     PCD |     0 |    1 | Page-level cache disable
     *       A |     0 |    1 | Is set if the entry has been accessed
     * Ignored |     0 |    1 | Ignored
     *     PAT |     0 |    1 | 1 = PAT is support, 0 = PAT is not supported
     *       G |     0 |    1 | 1 = The PTE is global, 0 = The PTE is local
     * Ignored |     0 |    3 | Ignored
     *
     * NOTE: G and Ignore are part of pt[n].middle, not pt[n].config!
     */
    pt[n].config =
        (0x01 << 3) | ((0x01 & pl) << 1) | ((0x01 & rw) << 1) | 0x01;
}
