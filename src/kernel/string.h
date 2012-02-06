#ifndef STRING_H
#define STRING_H

#include "stddef.h"

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
size_t strcspn(const char *s, const char *reject);
char *strchr(const char *s, int c);

#endif /* STRING_H */
