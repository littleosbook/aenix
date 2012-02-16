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
#include "vfs.h"
#include "aefs.h"
#include "process.h"
#include "page_frame_allocator.h"
#include "tss.h"
#include "stddef.h"
#include "keyboard.h"
#include "scheduler.h"
#include "process.h"
#include "kmalloc.h"
#include "vfs.h"
#include "devfs.h"

#define KINIT_ERROR_LOAD_FS 1
#define KINIT_ERROR_INIT_FS 2
#define KINIT_ERROR_INIT_PAGING 3
#define KINIT_ERROR_INIT_PFA 4
#define KINIT_ERROR_MALLOC_ROOT_VFS 5
#define KINIT_ERROR_INIT_VFS 6
#define KINIT_ERROR_INIT_SCHEDULER 7

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

static uint32_t add_device(char const *name, int (*get_vnode)(vnode_t *out))
{
    vnode_t *device = kmalloc(sizeof(vnode_t));
    if (device == NULL) {
        log_error("populate_devfs",
                  "Could not allocate vnode_t struct for device %s\n", name);
        return 1;
    }

    if (get_vnode(device)) {
        log_error("populate_devfs",
                  "Device %s could not generate vnode\n", name);
        return 1;
    }

    devfs_add_device(name, device);

    return 0;
}

static uint32_t populate_devfs()
{
    vfs_t *devfs = kmalloc(sizeof(vfs_t));
    if (devfs == NULL) {
        log_error("populate_devfs",
                  "Could not allocate vfs_t struct for devfs\n");
        return 1;
    }

    devfs_init(devfs);

    add_device("console", fb_get_vnode);
    add_device("keyboard", kbd_get_vnode);

    vfs_mount("/dev/", devfs);

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

    fb_init();

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

    vfs_t *aefs_vfs = kmalloc(sizeof(vfs_t));
    if (aefs_vfs == NULL) {
        return KINIT_ERROR_MALLOC_ROOT_VFS;
    }

    res = aefs_init(fs_paddr, fs_size, aefs_vfs);
    if (res != 0) {
        return KINIT_ERROR_INIT_FS;
    }

    res = vfs_mount("/", aefs_vfs);
    if (res != 0) {
        return KINIT_ERROR_INIT_VFS;
    }

    populate_devfs();

    res = scheduler_init();
    if (res != 0) {
        return KINIT_ERROR_INIT_SCHEDULER;
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

static void start_init()
{
    ps_t *init = process_create("/bin/init", scheduler_next_pid());
    if (init == NULL) {
        printf("ERROR: Could not create init!\n");
    } else {
        scheduler_add_runnable_process(init);
        scheduler_schedule();
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
            case KINIT_ERROR_MALLOC_ROOT_VFS:
                printf("ERROR: Could not allocate VFS node for root FS!\n");
                break;
            case KINIT_ERROR_INIT_VFS:
                printf("ERROR: Could not initialize virtual filesystem!\n");
                break;
            case KINIT_ERROR_INIT_SCHEDULER:
                printf("ERROR: Could not initialize scheduler!\n");
                break;
            default:
                printf("ERROR: Unknown error\n");
                break;
        }

        return 0xDEADDEAD;
    }

    log_info("kmain", "kernel initialized successfully!\n");

    start_init();

    return 0xDEADBEEF;
}
