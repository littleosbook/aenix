#include "mem.h"

uint32_t align_up(uint32_t n, uint32_t a)
{
    uint32_t m = n % a;
    if (m == 0) {
        return n;
    }
    return n + (a - m);
}

uint32_t align_down(uint32_t n, uint32_t a)
{
    return n - (n % a);
}
