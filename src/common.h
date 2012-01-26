#ifndef COMMON_H
#define COMMON_H

#define UNUSED_ARGUMENT(x) (void) x;

#define KERNEL_BASE_ADDR    0xC0000000

#define VIRTUAL_TO_PHYSICAL(addr) ((addr)-KERNEL_BASE_ADDR)
#define PHYSICAL_TO_VIRTUAL(addr) ((addr)+KERNEL_BASE_ADDR)

#define ALIGN(addr, alignment) (addr) + (alignment - ((addr) % alignment))

#endif /* COMMON_H */
