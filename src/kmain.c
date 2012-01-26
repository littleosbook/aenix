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
#include "serial.h"
#include "log.h"

void kinit(kernel_meminfo_t *mem, uint32_t boot_page_directory)
{
    UNUSED_ARGUMENT(mem);
    disable_interrupts();
    kmalloc_init(NEXT_ADDR(mem->kernel_virtual_end),
                 KERNEL_HEAP_SIZE);
    gdt_init();
    idt_init();
    pic_init();
    serial_init(COM1);
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

void log_memory_map(multiboot_info_t *mbinfo)
{
    /* From the GRUB multiboot manual section 3.3 boot information format
     * If flags[0] is set, then the fields mem_lower and mem_upper can be
     * accessed.
     * If flags[6] is set, then the fields mmap_length and mmap_addr can be
     * accessed, which contains a complete memory map.
     */
    if (mbinfo->flags & 0x00000001) {
        log_printf("size of lower memory: %u kB\n", mbinfo->mem_lower);
        log_printf("size of upper memory: %u kB\n", mbinfo->mem_upper);
        log_printf("\n");
    }

    if (mbinfo->flags & 0x00000020) {
        multiboot_memory_map_t *entry =
            (multiboot_memory_map_t *) mbinfo->mmap_addr;
        while ((uint32_t) entry < mbinfo->mmap_addr + mbinfo->mmap_length) {
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                log_printf("available memory: ");
            } else {
                log_printf("reserved memory:  ");
            }
            /* FIXME: printf should implement %llu */
            log_printf("address: %X length: %u\n",
                    (uint32_t) entry->addr, (uint32_t) entry->len);
            entry = (multiboot_memory_map_t *)
                (((uint32_t) entry) + entry->size + sizeof(entry->size));
        }
    }
    log_printf("\n");
}

void log_kernel_mem_info(kernel_meminfo_t *mem) {
    log_printf("kernel physical start: %X\n", mem->kernel_physical_start);
    log_printf("kernel physical end: %X\n", mem->kernel_physical_end);
    log_printf("kernel virtual start: %X\n", mem->kernel_virtual_start);
    log_printf("kernel virtual end: %X\n", mem->kernel_virtual_end);
    log_printf("\n");
}

void log_module_info(multiboot_info_t *mbinfo)
{
    uint32_t i;
    char *name;
    if (mbinfo->flags & 0x00000008) {
        log_printf("Number of modules: %u\n", mbinfo->mods_count);
        multiboot_module_t *module = (multiboot_module_t *) mbinfo->mods_addr;
        for (i = 0; i < mbinfo->mods_count; ++i, ++module) {
            if (module->cmdline == 0) {
                log_printf("module: no cmdline found\n");
            } else {
                name = (char *) PHYSICAL_TO_VIRTUAL(module->cmdline);
                log_printf("module %s\n", name);
            }
            log_printf("\tstart: %X\n", module->mod_start);
            log_printf("\tend: %X\n", module->mod_end);
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
	printf(
"=======================================================\n"
"       d8888 8888888888 888b    888 8888888 Y88b   d88P\n"
"      d88888 888        8888b   888   888    Y88b d88P \n"
"     d88P888 888        88888b  888   888     Y88o88P  \n"
"    d88P 888 8888888    888Y88b 888   888      Y888P   \n"
"   d88P  888 888        888 Y88b888   888      d888b   \n"
"  d88P   888 888        888  Y88888   888     d88888b  \n"
" d8888888888 888        888   Y8888   888    d88P Y88b \n"
"d88P     888 8888888888 888    Y888 8888888 d88P   Y88b\n"
"=======================================================\n");
    log_memory_map(mbinfo);
    log_kernel_mem_info(&mem);
    log_module_info(mbinfo);

    for (i = 0; i < 4; ++i, ++module)
        printf("m: %X -> %X\n", module, *module);
    module_entry_point();
    /*pit_set_callback(1, &display_tick); */

    return 0xDEADBEEF;
}
