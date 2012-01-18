#include "fb.h"
#include "gdt.h"
#include "pic.h"

#define UNUSED_ARGUMENT(x) (void) x

void test_fb()
{
    int i;
    fb_clear();
    for (i = 0; i < 25; i++) {
        fb_puts("BABA\n");
    }
    fb_puts("GEGE\n");
}

int kmain(void *mboot, unsigned int magic_number)
{
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);

    gdt_init();

    pic_init();

    test_fb();

    return 0xDEADBEEF;
}
