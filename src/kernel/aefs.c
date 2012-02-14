#include "aefs.h"
#include "string.h"
#include "log.h"
#include "paging.h"
#include "common.h"
#include "math.h"

#define ROOT_INODE_ID 1

struct aefs_block {
    char data[AEFS_BLOCK_SIZE];
} __attribute__((packed));
typedef struct aefs_block aefs_block_t;

static uint32_t fs_paddr;
static uint32_t fs_size;
static aefs_superblock_t *sb;
static aefs_inode_t *root_inode;
static aefs_block_t *fs_start;
static vnodeops_t vnodeops;
static vfsops_t vfsops;

static int aefs_root(vfs_t *vfs, vnode_t *vroot)
{
    UNUSED_ARGUMENT(vfs);

    vroot->v_op = &vnodeops;
    vroot->v_data = (uint32_t) root_inode;

    return 0;
}

static aefs_inode_t *get_inode(uint16_t inode_id)
{
    return ((aefs_inode_t *)(fs_start + 1)) + inode_id;
}

static aefs_block_t *get_block(uint32_t i)
{
    return fs_start + sb->start_block + i;
}

static int aefs_getattr(vnode_t *vnode, vattr_t *attr)
{
    aefs_inode_t *inode = (aefs_inode_t *) vnode->v_data;
    attr->file_size = AEFS_INODE_SIZE(inode);

    return 0;
}

static int aefs_read(vnode_t *vnode, void *buf, uint32_t count)
{
    uint32_t i, read = 0, to_read;
    aefs_inode_t *inode = (aefs_inode_t *) vnode->v_data;

    if (!AEFS_INODE_IS_REG(inode)) {
        return -1;
    }

    uint32_t size = minu(count, AEFS_INODE_SIZE(inode));
    uint32_t blocks = div_ceil(size, AEFS_BLOCK_SIZE);

    aefs_inode_t *current = inode;
    for (i = 0; i < blocks; ++i) {
        if (i % AEFS_INODE_NUM_BLOCKS == 0 && i != 0) {
            current = get_inode(current->inode_tail);
        }

        to_read = minu(size, AEFS_BLOCK_SIZE);
        memcpy(buf + read,
               get_block(current->blocks[i % AEFS_INODE_NUM_BLOCKS]),
               to_read);
        size -= to_read;
        read += to_read;
    }

    return read;
}

static int aefs_write(vnode_t *n, char const *s, size_t l)
{
    /* TODO: Not implemented yet! */
    UNUSED_ARGUMENT(n);
    UNUSED_ARGUMENT(s);
    UNUSED_ARGUMENT(l);

    log_error("aefs_write", "NOT IMPLEMENTED YET!\n");

    return -1;
}

static int aefs_lookup(vnode_t *dir, char const *name, vnode_t *res)
{
    uint32_t i, j;
    aefs_block_t *block;
    aefs_direntry_t *entry;
    aefs_inode_t *inode = (aefs_inode_t *) dir->v_data;

    if (!AEFS_INODE_IS_DIR(inode)) {
        log_error("aefs_lookup",
                  "dir is not a directory, looking for name %s\n",
                  name);
        return -1;
    }

    uint32_t num_blocks = div_ceil(AEFS_INODE_SIZE(inode), AEFS_BLOCK_SIZE);

    aefs_inode_t *current = inode;
    for (i = 0; i < num_blocks; ++i) {
        if (i % AEFS_INODE_NUM_BLOCKS == 0 && i != 0) {
            current = get_inode(current->inode_tail);
        }
        block = get_block(current->blocks[i % AEFS_INODE_NUM_BLOCKS]);
        entry = (aefs_direntry_t *) block;

        for (j = 0; j < AEFS_DIRENTRIES_PER_BLOCK; ++j, ++entry) {
            if (strncmp(entry->name, name, AEFS_FILENAME_MAX_LEN) == 0) {
                res->v_op = &vnodeops;
                res->v_data = (uint32_t) get_inode(entry->inode_id);
                return 0;
            }
        }
    }

    return -1;
}

static int aefs_open(vnode_t *node)
{
    UNUSED_ARGUMENT(node);
    /* NOOP for AEFS */
    return 0;
}


static int map_aefs_to_virtual_memory(uint32_t paddr, uint32_t size)
{
    uint32_t fs_vaddr, mapped_mem_size;
    fs_vaddr = pdt_kernel_find_next_vaddr(size);
    if (fs_vaddr == 0) {
        log_error("map_aefs_to_virtual_memory",
                  "Could not find virtual memory in kernel for AEFS."
                  "fs_paddr: %X, fs_size: %u\n",
                  paddr, size);
        return 0;
    }

    mapped_mem_size = pdt_map_kernel_memory(paddr, fs_vaddr, size,
                          PAGING_PL0, PAGING_READ_ONLY);
    if (mapped_mem_size < size) {
        log_error("map_aefs_to_virtual_memory",
                  "Could not map kernel memory for AEFS."
                  "fs_paddr: %X, fs_vaddr: %X, fs_size: %u, mapped_size: %u\n",
                  paddr, fs_vaddr, size, mapped_mem_size);
        return 0;
    }

    log_info("map_aefs_to_virtual_memory",
             "fs info: fs_vaddr: %X, fs_paddr: %X, fs_size: %u\n",
             fs_vaddr, fs_paddr, size);

    return fs_vaddr;
}

uint32_t aefs_init(uint32_t paddr, uint32_t size, vfs_t *vfs)
{
    uint32_t fs_vaddr;
    fs_paddr = paddr;
    fs_size = size;

    fs_vaddr = map_aefs_to_virtual_memory(paddr, size);

    if (fs_vaddr == 0) {
        log_error("aefs_init",
                  "Could not map AEFS to virtual memory\n");
        return 1;
    }

    fs_start = (aefs_block_t *) fs_vaddr;
    sb = (aefs_superblock_t *) fs_vaddr;

    if (sb->magic_number != AEFS_MAGIC_NUMBER) {
        log_error("aefs_init",
                  "Magic number of superblock is wrong: %X\n",
                  sb->magic_number);
        return 1;
    }

    root_inode = get_inode(ROOT_INODE_ID);

    vnodeops.vn_open = &aefs_open;
    vnodeops.vn_lookup = &aefs_lookup;
    vnodeops.vn_read = &aefs_read;
    vnodeops.vn_getattr = &aefs_getattr;
    vnodeops.vn_write = &aefs_write;

    vfsops.vfs_root = &aefs_root;

    vfs->vfs_next = NULL;
    vfs->vfs_op = &vfsops;

    return 0;
}
