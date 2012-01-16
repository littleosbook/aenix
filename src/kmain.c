#include "fb.h"

#define UNUSED_ARGUMENT(x) (void) x

int kmain(void *mboot, unsigned int magic_number)
{
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);
    write('h', 0, 0);
    write('e', 0, 1);
    write('l', 0, 2);
    write('l', 0, 3);
    write('o', 0, 4);
    return 0xDEADBEEF;
}
