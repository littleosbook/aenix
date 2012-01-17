#include "fb.h"

#define UNUSED_ARGUMENT(x) (void) x

int kmain(void *mboot, unsigned int magic_number)
{
    int i;
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);
    fb_clear();
    for (i = 0; i < 25; i++) {
        fb_puts("BABA\n");
    }
    fb_puts("GEGE\n");
    return 0xDEADBEEF;
}
