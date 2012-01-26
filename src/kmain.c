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
#include "kmalloc.h"

void kinit(kernel_meminfo_t *mem, uint32_t boot_page_directory)
{
    UNUSED_ARGUMENT(mem);
    disable_interrupts();
    kmalloc_init(NEXT_ADDR(mem->kernel_virtual_end),
                 KERNEL_HEAP_SIZE);
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
    multiboot_info_t *mbinfo = (multiboot_info_t *) PHYSICAL_TO_VIRTUAL(mbaddr);

    mbinfo->mmap_addr = PHYSICAL_TO_VIRTUAL(mbinfo->mmap_addr);
    mbinfo->mods_addr = PHYSICAL_TO_VIRTUAL(mbinfo->mods_addr);

    return mbinfo;
}

void display_memory_map(multiboot_info_t *mbinfo)
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
    printf("\n");
}

void display_kernel_mem_info(kernel_meminfo_t *mem) {
    printf("kernel physical start: %X\n", mem->kernel_physical_start);
    printf("kernel physical end: %X\n", mem->kernel_physical_end);
    printf("kernel virtual start: %X\n", mem->kernel_virtual_start);
    printf("kernel virtual end: %X\n", mem->kernel_virtual_end);
    printf("\n");
}

void display_module_info(multiboot_info_t *mbinfo)
{
    uint32_t i;
    char *name;
    if (mbinfo->flags & 0x00000008) {
        printf("Number of modules: %u\n", mbinfo->mods_count);
        multiboot_module_t *module = (multiboot_module_t *) mbinfo->mods_addr;
        for (i = 0; i < mbinfo->mods_count; ++i, ++module) {
            if (module->cmdline == 0) {
                printf("module: no cmdline found\n");
            } else {
                name = (char *) PHYSICAL_TO_VIRTUAL(module->cmdline);
                printf("module %s\n", name);
            }
            printf("\tstart: %X\n", module->mod_start);
            printf("\tend: %X\n", module->mod_end);
        }

    }
}

int kmain(uint32_t mbaddr, uint32_t magic_number, kernel_meminfo_t mem,
          uint32_t boot_page_directory)
{
    multiboot_info_t *mbinfo = remap_multiboot_info(mbaddr);
    void (*module_entry_point)(void) = (void (*)(void))0xC0400000;
    uint32_t *module = (uint32_t *) 0xC0400000;
    uint32_t i;
    fb_clear();

    if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("ERROR: magic number is wrong!\n");
        printf("magic_number: %u\n", magic_number);
        return 0xDEADDEAD;
    }

    kinit(&mem, boot_page_directory);
    printf("Welcome to aenix!\n");
    display_memory_map(mbinfo);
    display_kernel_mem_info(&mem);
    display_module_info(mbinfo);

    for (i = 0; i < 4; ++i, ++module)
        printf("m: %X -> %X\n", module, *module);
    module_entry_point();
    /*pit_set_callback(1, &display_tick); */

    return 0xDEADBEEF;
}
