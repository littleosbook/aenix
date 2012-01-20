#include "fb.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "interrupt.h"
#include "common.h"
#include "pit.h"
#include "multiboot.h"

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

void display_memory_info(multiboot_info_t *mbinfo)
{
    /* From the GRUB manual section 3.3 boot information format
     * If flags[0] is set, then the fields mem_lower and mem_upper can be 
     * accessed.
     * If flags[6] is set, then the fields mmap_length and mmap_addr can be
     * accessed, which contains a complete memory map.
     */
    if (mbinfo->flags & 0x00000001) {
        fb_puts("size of lower memory: ");
        fb_putui(mbinfo->mem_lower);
        fb_puts(" kB\n");
        fb_puts("size of upper memory: ");
        fb_putui(mbinfo->mem_upper);
        fb_puts(" kB\n");
        fb_putb('\n');
    }

    if (mbinfo->flags & 0x00000006) {
        multiboot_memory_map_t *entry = 
            (multiboot_memory_map_t *) mbinfo->mmap_addr;
        while ((uint32_t) entry < mbinfo->mmap_addr + mbinfo->mmap_length) {
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                fb_puts("Avaliable memory: ");
            } else {
                fb_puts("Reserved memory: ");
            }
            /* FIXME: This should fb_putull instead fb_putui, 
             * but fb_putull isn't written yet!
             */
            fb_puts("address: ");
            fb_putui((uint32_t) entry->addr);
            fb_puts(", length: ");
            fb_putui((uint32_t) entry->len);
            fb_putb('\n');
            entry = (multiboot_memory_map_t *) 
                (((uint32_t) entry) + entry->size + sizeof(entry->size));
        }
    }
}

int kmain(multiboot_info_t *mbinfo, uint32_t magic_number)
{
    fb_clear();

    if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
        fb_puts("ERROR: magic number is wrong!\n");
        fb_puts("magic_number: ");
        fb_putui(magic_number);
        return 0xDEADDEAD;
    }

    kinit();

    display_memory_info(mbinfo);

    /*pit_set_callback(1, &display_tick); */

    return 0xDEADBEEF;
}
