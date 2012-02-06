#include "math.h"

uint32_t div_ceil(uint32_t num, uint32_t den)
{
    return (num - 1) / den + 1;
}
