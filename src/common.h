#ifndef COMMON_H
#define COMMON_H

#include "constants.h"

#define UNUSED_ARGUMENT(x) (void) x;

#define PHYSICAL_TO_VIRTUAL(addr) ((addr)+KERNEL_START_VADDR)

#define NEXT_ADDR(addr) ((addr) + (4 - ((addr) % 4)))

#endif /* COMMON_H */
