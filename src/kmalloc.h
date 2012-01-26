#ifndef KMALLOC_H
#define KMALLOC_H

#include "stddef.h"
#include "stdint.h"

void kmalloc_init(uint32_t addr, size_t nbytes);
void *kmalloc(size_t);
void kfree(void *);

#endif /* KMALLOC_H */
