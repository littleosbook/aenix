#include "string.h"
#include "stdint.h"
#include "stdio.h"
#include "common.h"
#include "log.h"
#include "mem.h"
#include "constants.h"
#include "page_frame_allocator.h"

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
#define KERNEL_TMP_VADDR \
    (KERNEL_START_VADDR + KERNEL_TMP_PT_IDX * PT_ENTRY_SIZE)
#define KERNEL_PT_PDT_IDX VIRTUAL_TO_PDT_IDX(KERNEL_START_VADDR)

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
extern void invalidate_page_table_entry(uint32_t vaddr);


static void create_pdt_entry(pde_t *pdt,
                             uint32_t n,
                             uint32_t paddr,
                             uint8_t ps,
                             uint8_t rw,
                             uint8_t pl);
static void create_pt_entry(pte_t *pt,
                            uint32_t n,
                            uint32_t paddr,
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

static uint32_t get_pf_paddr(pte_t *pt, uint32_t pt_idx)
{
    pte_t *e = pt + pt_idx;
    uint32_t addr = e->high_addr;
    addr <<= 16;
    addr |= ((uint32_t) (e->middle & 0xF0) << 8);

    return addr;
}

static uint32_t kernel_map_temporary_memory(uint32_t paddr)
{
    create_pt_entry(kernel_pt, KERNEL_TMP_PT_IDX, paddr,
                    PAGING_READ_WRITE, PAGING_PL0);
    invalidate_page_table_entry(KERNEL_TMP_VADDR);
    return KERNEL_TMP_VADDR;
}

static uint32_t kernel_get_temporary_entry()
{
    return *((uint32_t *) &kernel_pt[KERNEL_TMP_PT_IDX]);
}

static void kernel_set_temporary_entry(uint32_t entry)
{
    kernel_pt[KERNEL_TMP_PT_IDX] = *((pte_t *) &entry);
    invalidate_page_table_entry(KERNEL_TMP_VADDR);
}

/* The given pdt must be mapped somwhere in the kernels page table,
 * otherwise it would have been impossible to use the pointer.
 *
 * Therefore to look up the physical address, we can simply use the kernel_pdt.
 */
static uint32_t get_pdt_paddr(pde_t *pdt)
{
    uint32_t pdt_vaddr = (uint32_t) pdt;
    uint32_t kpdt_idx = VIRTUAL_TO_PDT_IDX(pdt_vaddr);
    uint32_t kpt_paddr = get_pt_paddr(kernel_pdt, kpdt_idx);
    uint32_t kpt_idx = VIRTUAL_TO_PT_IDX(pdt_vaddr);

    uint32_t prev_tmp_entry = kernel_get_temporary_entry();
    uint32_t kpt_vaddr = kernel_map_temporary_memory(kpt_paddr);

    pte_t *kpt = (pte_t *) kpt_vaddr;
    uint32_t pdt_paddr = get_pf_paddr(kpt, kpt_idx);

    kernel_set_temporary_entry(prev_tmp_entry);

    return pdt_paddr;
}

uint32_t paging_init(uint32_t kernel_pdt_vaddr, uint32_t kernel_pt_vaddr)
{
    log_info("paging_init", "kernel_pdt_vaddr: %X, kernel_pt_vaddr: %X\n",
                     kernel_pdt_vaddr, kernel_pt_vaddr);
    kernel_pdt = (pde_t *) kernel_pdt_vaddr;
    kernel_pt = (pte_t *) kernel_pt_vaddr;
    return 0;
}

static uint32_t pt_kernel_find_next_vaddr(uint32_t pdt_idx,
                                          pte_t *pt, uint32_t size)
{
    uint32_t i, num_to_find, num_found = 0, org_i;
    num_to_find = align_up(size, FOUR_KB) / FOUR_KB;

    for (i = 0; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(pt+i) ||
            (pdt_idx == KERNEL_PT_PDT_IDX && i == KERNEL_TMP_PT_IDX)) {
            num_found = 0;
        } else {
            if (num_found == 0) {
                    org_i = i;
            }
            ++num_found;
            if (num_found == num_to_find) {
                return PDT_IDX_TO_VIRTUAL(pdt_idx) |
                       PT_IDX_TO_VIRTUAL(org_i);
            }
        }
    }
    return 0;
}

uint32_t pdt_kernel_find_next_vaddr(uint32_t size)
{
    uint32_t pdt_idx, pt_paddr, pt_vaddr, tmp_entry, vaddr = 0;
    /* TODO: support > 4MB sizes */

    pdt_idx = VIRTUAL_TO_PDT_IDX(KERNEL_START_VADDR);
    for (; pdt_idx < NUM_ENTRIES; ++pdt_idx) {
        if (IS_ENTRY_PRESENT(kernel_pdt + pdt_idx)) {
			tmp_entry = kernel_get_temporary_entry();

            pt_paddr = get_pt_paddr(kernel_pdt, pdt_idx);
            pt_vaddr = kernel_map_temporary_memory(pt_paddr);
            vaddr =
                pt_kernel_find_next_vaddr(pdt_idx, (pte_t *) pt_vaddr, size);

	        kernel_set_temporary_entry(tmp_entry);
        } else {
            /* no pdt entry */
            vaddr = PDT_IDX_TO_VIRTUAL(pdt_idx);
        }
        if (vaddr != 0) {
            return vaddr;
        }
    }

    return 0;
}

static uint32_t pt_map_memory(pte_t *pt,
			      uint32_t pdt_idx,
                              uint32_t paddr,
                              uint32_t vaddr,
                              uint32_t size,
                              uint8_t rw,
                              uint8_t pl)
{
    uint32_t pt_idx = VIRTUAL_TO_PT_IDX(vaddr);
    uint32_t mapped_size = 0;

    while (mapped_size < size && pt_idx < NUM_ENTRIES) {
        if (IS_ENTRY_PRESENT(pt + pt_idx)) {
            log_info("pt_map_memory",
                     "Entry is present: pt: %X, pt_idx: %u, pdt_idx: %u "
                     "pt[pt_idx]: %X\n",
                     pt, pt_idx, pdt_idx, pt[pt_idx]);
            return mapped_size;
        } else if(pdt_idx == KERNEL_PT_PDT_IDX && pt_idx == KERNEL_TMP_PT_IDX) {
            return mapped_size;
        }

        create_pt_entry(pt, pt_idx, paddr, rw, pl);

        paddr += PT_ENTRY_SIZE;
        mapped_size += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return mapped_size;
}

uint32_t pdt_map_memory(pde_t *pdt,
                        uint32_t paddr,
                        uint32_t vaddr,
                        uint32_t size,
                        uint8_t rw,
                        uint8_t pl)
{
    uint32_t pdt_idx;
    pte_t *pt;
    uint32_t pt_paddr, pt_vaddr, tmp_entry;
    uint32_t mapped_size = 0;
    uint32_t total_mapped_size = 0;
    size = align_up(size, PT_ENTRY_SIZE);

    while (size != 0) {
        pdt_idx = VIRTUAL_TO_PDT_IDX(vaddr);

        tmp_entry = kernel_get_temporary_entry();

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            pt_paddr = pfa_allocate(1);
            if (pt_paddr == 0) {
                log_error("pdt_map_memory",
                          "Couldn't allocate page frame for new page table."
                          "pdt_idx: %u, data vaddr: %X, data paddr: %X, "
                          "data size: %u\n",
                          pdt_idx, vaddr, paddr, size);
                return 0;
            }
            pt_vaddr = kernel_map_temporary_memory(pt_paddr);
            memset((void *) pt_vaddr, 0, PT_SIZE);
        } else {
            pt_paddr = get_pt_paddr(pdt, pdt_idx);
            pt_vaddr = kernel_map_temporary_memory(pt_paddr);
        }

        pt = (pte_t *) pt_vaddr;
        mapped_size =
            pt_map_memory(pt, pdt_idx, paddr, vaddr, size, rw, pl);

        if (mapped_size == 0) {
            log_error("pdt_map_memory",
                      "Could not map memory in page table. "
                      "pt: %X, paddr: %X, vaddr: %X, size: %u\n",
                      (uint32_t) pt, paddr, vaddr, size);
            kernel_set_temporary_entry(tmp_entry);
            return 0;
        }

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            create_pdt_entry(pdt, pdt_idx, pt_paddr, PS_4KB, rw, pl);
        }

        kernel_set_temporary_entry(tmp_entry);

        size -= mapped_size;
        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
    }

    return total_mapped_size;
}

uint32_t pdt_map_kernel_memory(uint32_t paddr,
                               uint32_t vaddr,
                               uint32_t size,
                               uint8_t rw,
                               uint8_t pl)
{
    return pdt_map_memory(kernel_pdt, paddr, vaddr,
                          size, rw, pl);
}

static uint32_t pt_unmap_memory(pte_t *pt,
			        uint32_t pdt_idx,
                                uint32_t vaddr,
                                uint32_t size)
{
    uint32_t pt_idx = VIRTUAL_TO_PT_IDX(vaddr);
    uint32_t freed_size = 0;

    while (freed_size < size && pt_idx < NUM_ENTRIES) {
        if (pdt_idx == KERNEL_PT_PDT_IDX && pt_idx == KERNEL_TMP_PT_IDX) {
            /* can't touch this */
            return freed_size;
        }
        if (IS_ENTRY_PRESENT(pt + pt_idx)) {
            memset(pt + pt_idx, 0, sizeof(pte_t));
            invalidate_page_table_entry(vaddr);
        }

        freed_size += PT_ENTRY_SIZE;
        vaddr += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return freed_size;
}

uint32_t pdt_unmap_memory(pde_t *pdt, uint32_t vaddr, uint32_t size)
{
    uint32_t pdt_idx, pt_paddr, pt_vaddr, tmp_entry;

    uint32_t freed_size = 0;
    uint32_t end_vaddr;

    size = align_up(size, PT_ENTRY_SIZE);
    end_vaddr = vaddr + size;

    while (vaddr < end_vaddr) {
        pdt_idx = VIRTUAL_TO_PDT_IDX(vaddr);

        if (!IS_ENTRY_PRESENT(pdt + pdt_idx)) {
            vaddr = align_up(vaddr, PDT_ENTRY_SIZE);
            continue;
        }

        pt_paddr = get_pt_paddr(pdt, pdt_idx);
        tmp_entry = kernel_get_temporary_entry();

        pt_vaddr = kernel_map_temporary_memory(pt_paddr);

        freed_size =
            pt_unmap_memory((pte_t *) pt_vaddr, pdt_idx, vaddr, size);

        kernel_set_temporary_entry(tmp_entry);

        if (freed_size == PDT_ENTRY_SIZE) {
            if (pdt_idx != KERNEL_PT_PDT_IDX) {
                pfa_free(pt_paddr);
                memset(pdt + pdt_idx, 0, sizeof(pde_t));
            }
        }

        vaddr += freed_size;
    }

    return freed_size;
}

uint32_t pdt_unmap_kernel_memory(uint32_t virtual_addr, uint32_t size)
{
    return pdt_unmap_memory(kernel_pdt, virtual_addr, size);
}

/* OUT paddr: The physical address for the PDT */
pde_t *pdt_create(uint32_t *out_paddr)
{
    pde_t *pdt;
    *out_paddr = 0;
    uint32_t pdt_paddr = pfa_allocate(1);
    uint32_t pdt_vaddr = pdt_kernel_find_next_vaddr(PDT_SIZE);
    uint32_t size = pdt_map_kernel_memory(pdt_paddr, pdt_vaddr, PDT_SIZE,
                                          PAGING_READ_WRITE, PAGING_PL0);
    if (size < PDT_SIZE) {
        /* Since PDT_SIZE is the size of one frame, size must either be equal
         * to PDT_SIZE or 0
         */
        pfa_free(pdt_paddr);
        return NULL;
    }

    pdt = (pde_t *) pdt_vaddr;

    memset(pdt, 0, PDT_SIZE);

    *out_paddr = pdt_paddr;
    return pdt;
}

void pdt_delete(pde_t *pdt)
{
    uint32_t i, pdt_paddr;
    for (i = 0; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(pdt + i) && IS_ENTRY_PAGE_TABLE(pdt + i)) {
            pfa_free(get_pt_paddr(pdt, i));
        }
    }

    pdt_paddr = get_pdt_paddr(pdt);
    pfa_free(pdt_paddr);
}

void pdt_set(uint32_t pdt_paddr);
void pdt_load_process_pdt(pde_t *pdt, uint32_t pdt_paddr)
{
    uint32_t i;

    for (i = KERNEL_PDT_IDX; i < NUM_ENTRIES; ++i) {
        if (IS_ENTRY_PRESENT(kernel_pdt + i)) {
            pdt[i] = kernel_pdt[i];
        }
    }

    pdt_set(pdt_paddr);
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
        (0x01 << 3) | ((0x01 & pl) << 2) | ((0x01 & rw) << 1) | 0x01;
}
