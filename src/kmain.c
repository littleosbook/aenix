#include "fb.h"

#define UNUSED_ARGUMENT(x) (void) x

int kmain(void *mboot, unsigned int magic_number)
{
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);
    fb_clear();
    fb_write('h', 0, 0);
    fb_write('e', 0, 1);
    fb_write('l', 0, 2);
    fb_write('l', 0, 3);
    fb_write('o', 0, 4);
    return 0xDEADBEEF;
}
