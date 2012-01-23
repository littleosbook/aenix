#include "paging.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"

#define NUM_ENTRIES 1024
#define PDT_SIZE NUM_ENTRIES * sizeof(pde_t)
#define PT_SIZE  NUM_ENTRIES * sizeof(pte_t)

#define VIRTUAL_TO_PDT_IDX(a) ((a >> 20) & 0x3FF)

/* pde: page directory entry */
struct pde {
    uint8_t config;
    uint8_t low_addr; /* only the highest 4 bits are used */
    uint16_t high_addr;
} __attribute__((packed));
typedef struct pde pde_t;

struct pte {
    uint8_t config;
    uint8_t middle; /* only the highest 4 bits and the lowest bit are used */
    uint16_t high_addr;
    } __attribute__((packed));
typedef struct pte pte_t;

/**
 * Creates an entry in the page descriptor table at the specified index.
 * THe entry will point to the given PTE.
 *
 * @param pdt   The page descriptor table
 * @param n     The index in the PDT
 * @param pt    The address to the first entry in the page table
 */
static void create_pdt_entry(pde_t *pdt, uint32_t n, pte_t *pte)
{
    uint32_t addr = (uint32_t) pte;
    /* Since page tables are aligned at 4kB boundaries, we only need to store 
     * the 20 highest bits */
    /* The lower 4 bits */
    pdt[n].low_addr  = ((addr >> 12) & 0xF) << 4;
    pdt[n].high_addr = ((addr >> 16) & 0xFFFF);

    /*
     * name    | value | size | desc
     * ---------------------------
     *       P |     1 |    1 | If the entry is present or not
     *     R/W |     1 |    1 | Read/Write:
     *                              0 = Read-only
     *                              1 = Write and Read
     *     U/S |     0 |    1 | User/Supervisor: 
     *                              0 = PL3 can't access 
     *                              1 = PL3 can access
     *     PWT |     1 |    1 | Page-level write-through: 
     *                              0 = writes are cached
     *                              1 = writes are not cached
     *     PCD |     0 |    1 | Page-level cache disable
     *       A |     0 |    1 | Is set if the entry has been accessed
     * Ignored |     0 |    1 | Ignored
     *      PS |     0 |    1 | Page size:
     *                              0 = address point to pt entry, 
     *                              1 = address points to 4 MB page
     * Ignored |     0 |    4 | Ignored
     *
     * NOTE: Ignored is not part of pdt[n].config!
     */
    pdt[n].config = (0x01 << 3) | (0x01 << 1) | 0x01;
}

/**
 * Creates a new entry at the specified index in the given page table.
 *
 * @param pt    The page table
 * @param n     The index in the page table to create the entry at
 * @param addr  The addres to the page frame
 */
static void create_pt_entry(pte_t *pt, uint32_t n, uint32_t addr)
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
     *     R/W |     1 |    1 | Read/Write:
     *                              0 = Read-only
     *                              1 = Write and Read
     *     U/S |     0 |    1 | User/Supervisor: 
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
    pt[n].config = (0x01 << 3) | (0x01 << 1) | 0x01;
}

extern void pdt_set(uint32_t pdt_addr); /* defined in paging_asm.s */

void paging_init(uint32_t end_of_kernel)
{
    uint32_t i, cr3 = 0;
    pde_t *pdt = (pde_t *) end_of_kernel;
    pte_t *pt = (pte_t *) (end_of_kernel + PDT_SIZE);

    memset(pdt, 0, PDT_SIZE);
    memset(pt, 0, PT_SIZE);

    /* identity paging for first 4 MB */
    for (i = 0; i < 1024; ++i) {
        create_pt_entry(pt, i, 0x00000000 + i * 4096);
    }

    create_pdt_entry(pdt, 0, pt);

    printf("pdt: %X\n", (uint32_t) pdt);
    printf("pt: %X\n", (uint32_t) pt);
    printf("pdt[0].config %X\n", pdt[0].config);
    printf("pdt[0].low_addr %X\n", pdt[0].low_addr);
    printf("pdt[0].high_addr %X\n", pdt[0].high_addr);
    
    cr3 = ((uint32_t) pdt & 0xFFFFF000) | (0x01 << 3);
    pdt_set(cr3);
}
