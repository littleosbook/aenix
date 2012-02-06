#include "tss.h"

tss_t tss;

uint32_t tss_init()
{
    return (uint32_t) &tss;
}
