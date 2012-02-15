#include "math.h"

uint32_t div_ceil(uint32_t num, uint32_t den)
{
    return (num - 1) / den + 1;
}

uint32_t minu(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

uint32_t maxu(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}
