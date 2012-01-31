#include "kmalloc.h"
#include "stdio.h"
#include "log.h"

#define MIN_BLOCK_SIZE  1024 /* in units */

/* malloc() and free() as implemented by K&R */

typedef double align;

union header {
    struct {
        union header *next;
        size_t size; /* size in "number of header units" */
    } s;
    align x; /* force alignment of blocks, never use */
};
typedef union header header_t;

/* empty list to get started */
static header_t base;
/* start of free list */
static header_t *freep = 0;

/* address of next heap block */
static uint32_t next_heap_addr;

static void *acquire_more_heap(size_t nunits);

void kmalloc_init(uint32_t addr)
{
    if (freep == 0) {
        /* no free list yet */
        base.s.next = freep = &base;
        base.s.size = 0;
    }
    next_heap_addr = addr;
}

void *kmalloc_align(size_t nbytes, size_t alignment)
{
    header_t *p, *prevp;
    size_t nunits;

    if (nbytes == 0)
        return 0;

    nunits = (nbytes+sizeof(header_t)-1)/sizeof(header_t) + 1;
    prevp = freep;

    for (p = prevp->s.next; ; prevp = p, p = p->s.next) {
        if (p->s.size >= nunits) {
            /* the block is big enough */
            if (p->s.size == nunits) {
                /* exactly */
                prevp->s.next = p->s.next;
            } else {
                /* allocate at the tail end of the block and create a new
                 * header in front of allocated block */
                p->s.size -= nunits;
                p += p->s.size; /* make p point to new header */
                p->s.size = nunits;
            }
            freep = prevp;
            log_printf("kmalloc: %X (header_t: %X)\n", (uint32_t)(p+1), (uint32_t)p);
            return (void *)(p+1);
        }
        if (p == freep) {
            /* wrapped around free list */
            if ((p = acquire_more_heap(nunits)) == 0) {
                log_printf("ERROR: kmalloc cannot acquire more memory memory %u",
                        nbytes);
                return 0;
            }
        }
    }
}

void *kmalloc(size_t nbytes)
{
    kmalloc_align(nbytes, 1);
}

static void *acquire_more_heap(size_t nunits)
{
    header_t *p;

    if (nunits < MIN_BLOCK_SIZE) {
        nunits = MIN_BLOCK_SIZE;
    }

    log_printf("acquire_more_heap: %X %u units\n", next_heap_addr, nunits);

    p = (header_t *) next_heap_addr;
    p->s.size = nunits;

    next_heap_addr += nunits * sizeof(header_t);

    kfree((void *)(p+1));

    return freep;
}

void kfree(void * ap)
{
    header_t *bp, *p;

    if (ap == 0)
        return;

    log_printf("kfree: %X\n", (uint32_t)ap);

    /* point to block header */
    bp = (header_t *)ap - 1;
    for (p = freep; !(bp > p && bp < p->s.next); p = p->s.next) {
        if (p >= p->s.next && (bp > p || bp < p->s.next)) {
            /* freed block at start of end of arena */
            break;
        }
    }

    if (bp + bp->s.size == p->s.next) {
        /* join to upper nbr */
        bp->s.size += p->s.next->s.size;
        bp->s.next = p->s.next->s.next;
    } else {
        bp->s.next = p->s.next;
    }

    if (p + p->s.size == bp) {
        /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.next = bp->s.next;
    } else {
        p->s.next = bp;
    }
    freep = p;
}
