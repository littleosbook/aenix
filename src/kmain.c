#include "fb.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "interrupt.h"

#define UNUSED_ARGUMENT(x) (void) x

void kinit()
{
    disable_interrupts();
    gdt_init();
    pic_init();
    idt_init();
    enable_interrupts();
}

int kmain(void *mboot, unsigned int magic_number)
{
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);

    kinit();
    fb_clear();

    return 0xDEADBEEF;
}
