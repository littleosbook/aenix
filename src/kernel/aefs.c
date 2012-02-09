#include "aefs.h"
#include "string.h"
#include "log.h"
#include "paging.h"
#include "common.h"

/*static uint32_t fs_paddr;*/
/*static uint32_t fs_size;*/
/*static inode_t *root;*/

uint32_t aefs_init(uint32_t paddr, uint32_t size)
{
    UNUSED_ARGUMENT(paddr);
    UNUSED_ARGUMENT(size);
    /*uint32_t fs_vaddr, mapped_mem_size;*/
    /*fs_paddr = paddr;*/
    /*fs_size = size;*/
    /*fs_vaddr = pdt_kernel_find_next_vaddr(fs_size);*/
    /*if (fs_vaddr == 0) {*/
        /*log_error("fs_init",*/
                  /*"Could not find virtual memory in kernel for FS."*/
                  /*"fs_paddr: %X, fs_size: %u\n",*/
                  /*fs_paddr, fs_size);*/
        /*return 1;*/
    /*}*/

    /*mapped_mem_size = pdt_map_kernel_memory(fs_paddr, fs_vaddr, fs_size,*/
                          /*PAGING_PL0, PAGING_READ_ONLY);*/
    /*if (mapped_mem_size < fs_size) {*/
        /*log_error("fs_init",*/
                  /*"Could not map kernel memory for FS."*/
                  /*"fs_paddr: %X, fs_vaddr: %X, fs_size: %u, mapped_size: %u\n",*/
                  /*fs_paddr, fs_vaddr, fs_size, mapped_mem_size);*/
        /*return 1;*/
    /*}*/

    /*root = (inode_t *) fs_vaddr;*/

    /*log_info("fs_init", "fs_vaddr: %X, fs_paddr: %X, fs_size: %u\n",*/
             /*fs_vaddr, fs_paddr, fs_size);*/

    /*return 0;*/
    return 0;
}

    /*uint32_t num_files, i;*/

    /*if (dir->type != FILETYPE_DIR) {*/
        /*return NULL;*/
    /*}*/

    /*num_files = dir->size / sizeof(direntry_t);*/
    /*direntry_t *entry = (direntry_t *) (dir+1);*/
    /*for (i = 0; i < num_files; ++i, ++entry) {*/
        /*if (strncmp(path, entry->name, fname_len) == 0) {*/
            /*return (inode_t *) ((uint32_t) root + entry->location);*/
        /*}*/
    /*}*/

    /*size_t path_len = strlen(path);*/
    /*size_t slash_index = 0;*/
    /*inode_t *node = root;*/

    /*if (path_len == 0) {*/
        /*return NULL;*/
    /*}*/

    /*if (path[0] != '/') {*/
        /*return NULL;*/
    /*}*/

    /*while (strlen(path) != 0 && node != NULL) {*/
        /*++path; [> move past the '/' character <]*/
        /*slash_index = strcspn(path, "/");*/
        /*node = find_inode_in_dir(node, path, slash_index);*/
        /*path += slash_index;*/
    /*}*/

    /*return node;*/
