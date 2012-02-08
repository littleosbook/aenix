#include "stdio.h"
#include "fb.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "interrupt.h"
#include "constants.h"
#include "common.h"
#include "pit.h"
#include "multiboot.h"
#include "paging.h"
#include "kernel.h"
#include "serial.h"
#include "log.h"
#include "fs.h"
#include "process.h"
#include "page_frame_allocator.h"
#include "tss.h"
#include "stddef.h"
#include "keyboard.h"
#include "scheduler.h"
#include "process.h"

#define KINIT_ERROR_LOAD_FS 1
#define KINIT_ERROR_INIT_FS 2
#define KINIT_ERROR_INIT_PAGING 3
#define KINIT_ERROR_INIT_PFA 4

/* Gets the physical address of the filesystem, which is the address of the
 * only GRUB module loaded
 *
 * @param mbinfo A pointer to the multiboot info struct
 * @param size OUT The size of the fs in bytes will be written to his ptr
 *
 * @return The physical address of the filesystem
 */
static uint32_t get_fs_paddr(const multiboot_info_t *mbinfo, uint32_t *size)
{
    *size = 0;
    if (mbinfo->flags & 0x00000008) {
        if (mbinfo->mods_count != 1) {
            return 0;
        }

        multiboot_module_t *module = (multiboot_module_t *) mbinfo->mods_addr;
        *size = module->mod_end - module->mod_start;
        return module->mod_start;
    }

    return 0;
}

static uint32_t kinit(kernel_meminfo_t *mem,
                  const multiboot_info_t *mbinfo,
                  uint32_t kernel_pdt_vaddr,
                  uint32_t kernel_pt_vaddr)
{
    uint32_t fs_paddr, fs_size;
    uint32_t res;
    uint32_t tss_vaddr;
    disable_interrupts();

    fs_paddr = get_fs_paddr(mbinfo, &fs_size);
    if (fs_paddr == 0 && fs_size == 0) {
        return KINIT_ERROR_LOAD_FS;
    }

    tss_vaddr = tss_init();
    gdt_init(tss_vaddr);
    idt_init();
    pic_init();
    kbd_init();
    serial_init(COM1);
    pit_init();

    res = paging_init(kernel_pdt_vaddr, kernel_pt_vaddr);
    if (res != 0) {
        return KINIT_ERROR_INIT_PAGING;
    }

    res = pfa_init(mbinfo, mem, fs_paddr, fs_size);
    if (res != 0) {
        return KINIT_ERROR_INIT_PFA;
    }

    res = fs_init(fs_paddr, fs_size);
    if (res != 0) {
        return KINIT_ERROR_INIT_FS;
    }

    enable_interrupts();
    return 0;
}

static multiboot_info_t *remap_multiboot_info(uint32_t mbaddr)
{
    multiboot_info_t *mbinfo = (multiboot_info_t *) PHYSICAL_TO_VIRTUAL(mbaddr);

    mbinfo->mmap_addr = PHYSICAL_TO_VIRTUAL(mbinfo->mmap_addr);
    mbinfo->mods_addr = PHYSICAL_TO_VIRTUAL(mbinfo->mods_addr);

    return mbinfo;
}

/* defined in enter_user_mode.s */
void enter_user_mode(uint32_t init_addr, uint32_t stack_addr);

static void start_init()
{
    ps_t *init = process_create("/bin/init", 1);
    if (init == NULL) {
        printf("ERROR: Could not create init!\n");
    } else {
        scheduler_switch_to_process(init);
        enter_user_mode(init->code_vaddr, init->stack_vaddr);
    }
}

int kmain(uint32_t mbaddr, uint32_t magic_number, kernel_meminfo_t mem,
          uint32_t kernel_pdt_vaddr, uint32_t kernel_pt_vaddr)
{
    uint32_t res;
    multiboot_info_t *mbinfo = remap_multiboot_info(mbaddr);

    fb_clear();

    if (magic_number != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("ERROR: magic number is wrong!\n");
        printf("magic_number: %u\n", magic_number);
        return 0xDEADDEAD;
    }

    res = kinit(&mem, mbinfo, kernel_pdt_vaddr, kernel_pt_vaddr);
    if (res != 0) {
        switch (res) {
            case KINIT_ERROR_LOAD_FS:
                printf("ERROR: Could not load filesystem!\n");
                break;
            case KINIT_ERROR_INIT_FS:
                printf("ERROR: Could not initialize filesystem!\n");
                break;
            case KINIT_ERROR_INIT_PAGING:
                printf("ERROR: Could not initialize paging!\n");
                break;
            case KINIT_ERROR_INIT_PFA:
                printf("ERROR: Could not initialize page frame allocator!\n");
                break;
            default:
                printf("ERROR: Unknown error\n");
                break;
        }
        return 0xDEADDEAD;
    }

    start_init();

    return 0xDEADBEEF;
}
