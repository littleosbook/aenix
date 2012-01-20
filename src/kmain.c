#include "fb.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "interrupt.h"
#include "common.h"
#include "pit.h"

void kinit()
{
    disable_interrupts();
    gdt_init();
    pic_init();
    idt_init();
    pit_init();
    enable_interrupts();
}

void display_tick()
{
    fb_putb('.');
}

int kmain(void *mboot, unsigned int magic_number)
{
    UNUSED_ARGUMENT(mboot);
    UNUSED_ARGUMENT(magic_number);

    kinit();
    fb_clear();
    pit_set_callback(1, &display_tick); 

    return 0xDEADBEEF;
}
