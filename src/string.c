#include "string.h"
#include "stdint.h"

void *memset(void *s, int c, size_t n)
{
    uint8_t *base = (uint8_t *) s;
    uint8_t *p;
    uint8_t cb = (uint8_t) (c & 0xFF);

    for (p = base; p < base + n; ++p) {
        *p = cb;
    }

    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *from = (uint8_t *) dest;
    uint8_t *to = (uint8_t *) src;
    size_t i;

    for(i = 0; i < n; ++i) {
        *to++ = *from++;
    }

    return dest;
}
