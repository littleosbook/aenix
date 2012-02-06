#include "tss.h"
#include "log.h"

static tss_t tss;

uint32_t tss_init()
{
    return (uint32_t) &tss;
}

void tss_set_kernel_stack(uint16_t segsel, uint32_t vaddr)
{
    tss.esp0 = vaddr;
    tss.ss0 = segsel;
}
