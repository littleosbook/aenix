#include "string.h"
#include "stdint.h"
#include "stddef.h"

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
    uint8_t *to = (uint8_t *) dest;
    uint8_t *from = (uint8_t *) src;
    size_t i;

    for(i = 0; i < n; ++i) {
        *to++ = *from++;
    }

    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    size_t len1 = strlen(s1), len2 = strlen(s2);

    if (len1 < len2) {
        return -1;
    }

    if (len1 > len2) {
        return 1;
    }

    for (; *s1; ++s1, ++s2) {
        if (*s1 == *s2) {
            continue;
        } else if (*s1 < *s2) {
            return -1;
        } else {
            return 1;
        }
    }

    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;

    for (i = 0; i < n; ++i) {
        if (*s1 == *s2) {
            continue;
        }

        if (*s1 == '\0' && *s2 != '\0') {
            return -1;
        }

        if (*s2 == '\0' && *s1 != '\0') {
            return 1;
        }

        if (*s1 < *s2) {
            return -1;
        } else {
            return 1;
        }
    }

    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;

    for (; *s; ++s) {
        ++len;
    }

    return len;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t l = 0;

    for (; *s; ++s) {
        if (strchr(reject, (int) *s) != NULL) {
            break;
        }
        ++l;
    }

    return l;
}

char *strchr(const char *s, int c)
{
    for (; *s; ++s) {
        if (*s == (char) c) {
            return (char *) s; /* ugly, but it follows the standard */
        }
    }

    return NULL;
}
