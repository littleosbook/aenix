#include "stdint.h"
#include "gdt.h"

#define SEGMENT_BASE    0
#define SEGMENT_LIMIT   0xFFFFF

#define CODE_RX_TYPE    0xA
#define DATA_RW_TYPE    0x2

#define GDT_NUM_ENTRIES 3

struct gdt_entry {
    uint16_t limit_low;     /* The lower 16 bits of the limit */
    uint16_t base_low;      /* The lower 16 bits of the base */
    uint8_t  base_mid;      /* The next 8 bits of the base */
    uint8_t  access;        /* Contains access flags */
    uint8_t  granularity;   /* Specify granularity, and 4 bits of limit */
    uint8_t  base_high;     /* The last 8 bits of the base; */
} __attribute__((packed));  /* It needs to be packed like this, 64 bits */
typedef struct gdt_entry gdt_entry_t;

struct gdt_ptr {
    uint16_t limit;          /* Size of gdt table in bytes*/
    uint32_t base;           /* Address to the first gdt entry */
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];

/* external assembly function to set the gdt */
void gdt_load_and_set(uint32_t);
static void gdt_create_entry(uint32_t n, uint8_t pl, uint8_t type);

void gdt_init()
{
	gdt_ptr_t gdt_ptr;
    gdt_ptr.limit   = sizeof(gdt_entry_t)*GDT_NUM_ENTRIES;
    gdt_ptr.base    = (uint32_t)&gdt_entries;

    /* the null entry */
    gdt_create_entry(0, 0, 0);
    /* kernel mode code segment */
    gdt_create_entry(1, PL0, CODE_RX_TYPE);
    /* kernel mode data segment */
    gdt_create_entry(2, PL0, DATA_RW_TYPE);
    /* user mode code segment */
    /* user mode data segment */

    gdt_load_and_set((uint32_t)&gdt_ptr);
}



static void gdt_create_entry(uint32_t n, uint8_t pl, uint8_t type)
{
    gdt_entries[n].base_low     = (SEGMENT_BASE & 0xFFFF);
    gdt_entries[n].base_mid     = (SEGMENT_BASE >> 16) & 0xFF;
    gdt_entries[n].base_high    = (SEGMENT_BASE >> 24) & 0xFF;

    gdt_entries[n].limit_low    = (SEGMENT_LIMIT & 0xFFFF);
 
    /*
     * name | value | size | desc
     * ---------------------------
     * G    |     1 |    1 | granularity, size of segment unit
     * D/B  |     1 |    1 | size of operation size, 0 = 16 bits, 1 = 32 bits
     * L    |     0 |    1 | 1 = 64 bit code
     * AVL  |     0 |    1 | "available for use by system software"
     * LIM  |   0xF |    4 | the four highest bits of segment limit
     */
    gdt_entries[n].granularity  |= (0x01 << 7) | (0x01 << 6) | 0x0F;

    /*
     * name | value | size | desc
     * ---------------------------
     * P    |     1 |    1 | segment present in memory
     * DPL  |    pl |    2 | privilege level
     * S    |     1 |    1 | descriptor type, 0 = system, 1 = code or data
     * Type |  type |    4 | segment type, how the segment can be accessed
     */
    gdt_entries[n].access =
        (0x01 << 7) | ((pl & 0x03) << 5) | (0x01 << 4) | (type & 0x0F);
}

