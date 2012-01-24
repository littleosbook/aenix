#include "stdio.h"
#include "fb.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "interrupt.h"
#include "common.h"
#include "pit.h"
#include "multiboot.h"
#include "paging.h"
#include "kernel.h"

void kinit(kernel_meminfo_t *mem, uint32_t boot_page_directory)
{
    UNUSED_ARGUMENT(mem);
    disable_interrupts();
    gdt_init();
    pic_init();
    idt_init();
    pit_init();
	paging_init(boot_page_directory);
    enable_interrupts();
}

void display_tick()
{
    printf(".");
}

multiboot_info_t *remap_multiboot_info(uint32_t mbaddr)
{
    multiboot_info_t *mbinfo = (multiboot_info_t *) (mbaddr + KERNEL_BASE_ADDR);

    mbinfo->mmap_addr += KERNEL_BASE_ADDR;

    return mbinfo;
}

void display_memory_info(multiboot_info_t *mbinfo, kernel_meminfo_t *mem)
{
    /* From the GRUB multiboot manual section 3.3 boot information format
     * If flags[0] is set, then the fields mem_lower and mem_upper can be 
     * accessed.
     * If flags[6] is set, then the fields mmap_length and mmap_addr can be
     * accessed, which contains a complete memory map.
     */
    if (mbinfo->flags & 0x00000001) {
        printf("size of lower memory: %u kB\n", mbinfo->mem_lower);
        printf("size of upper memory: %u kB\n", mbinfo->mem_upper);
        printf("\n");
    }

    if (mbinfo->flags & 0x00000020) {
        multiboot_memory_map_t *entry = 
            (multiboot_memory_map_t *) mbinfo->mmap_addr;
        while ((uint32_t) entry < mbinfo->mmap_addr + mbinfo->mmap_length) {
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                printf("available memory: ");
            } else {
                printf("reserved memory:  ");
            }
            /* FIXME: printf should implement %llu */
            printf("address: %X length: %u\n", 
                    (uint32_t) entry->addr, (uint32_t) entry->len);
            entry = (multiboot_memory_map_t *) 
                (((uint32_t) entry) + entry->size + sizeof(entry->size));
        }
    }

    printf("kernel physical start: %X\n", mem->kernel_physical_start);
    printf("kernel physical end: %X\n", mem->kernel_physical_end);
    printf("kernel virtual start: %X\n", mem->kernel_virtual_start);
    printf("kernel virtual end: %X\n", mem->kernel_virtual_end);
}

int kmain(uint32_t mbaddr, uint32_t magic_number, kernel_meminfo_t mem,
          uint32_t boot_page_directory)
{
    multiboot_info_t *mbinfo = remap_multiboot_info(mbaddr);
    fb_clear();

    if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("ERROR: magic number is wrong!\n");
        printf("magic_number: %u\n", magic_number);
        return 0xDEADDEAD;
    }

    kinit(&mem, boot_page_directory);

    printf("Welcome to aenix!\n");
    display_memory_info(mbinfo, &mem);

    /*pit_set_callback(1, &display_tick); */

    return 0xDEADBEEF;
}
