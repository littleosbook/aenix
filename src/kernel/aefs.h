#ifndef FS_H
#define FS_H

#include "stdint.h"
#include "vfs.h"

#define AEFS_FILENAME_MAX_LEN 254

#define AEFS_FILETYPE_REG 0
#define AEFS_FILETYPE_DIR 1

#define AEFS_BLOCK_SIZE 1024 /* in bytes */
#define AEFS_INODE_NUM_BLOCKS 5

#define AEFS_MAGIC_NUMBER 0xAE12AE34

/* sizeof(inode_t) == 16 bytes */
struct aefs_inode {
    uint8_t type;
    uint8_t size_high;
    uint16_t size_low; /* in bytes */
    uint16_t blocks[AEFS_INODE_NUM_BLOCKS];
    uint16_t inode_tail;
} __attribute__((packed));
typedef struct aefs_inode aefs_inode_t;

/* sizeof(inode_list_t) == 16 bytes */
struct aefs_inode_list {
    uint32_t padding;
    uint16_t blocks[AEFS_INODE_NUM_BLOCKS];
    uint16_t inode_tail;
} __attribute__((packed));
typedef struct aefs_inode_list aefs_inode_list_t;

/* sizeof(direntry_t) == 256 bytes */
struct aefs_direntry {
    char name[AEFS_FILENAME_MAX_LEN];
    uint16_t inode_id;
} __attribute__((packed));
typedef struct aefs_direntry aefs_direntry_t;

/* sizeof(superblock_t) == 12 bytes */
struct aefs_superblock {
    uint32_t magic_number;
    uint16_t num_inodes;
    uint16_t start_block;
} __attribute__((packed));
typedef struct aefs_superblock aefs_superblock_t;

#define AEFS_INODE_SIZE(inode) ((((uint32_t) (inode)->size_high) << 16) | \
                                ((inode)->size_low))
#define AEFS_DIRENTRIES_PER_BLOCK (AEFS_BLOCK_SIZE/sizeof(aefs_direntry_t))

#define AEFS_INODE_IS_REG(inode) ((inode)->type == AEFS_FILETYPE_REG)
#define AEFS_INODE_IS_DIR(inode) ((inode)->type == AEFS_FILETYPE_DIR)

uint32_t aefs_init(uint32_t fs_paddr, uint32_t fs_size, vfs_t *vfs);

#endif /* FS_H */
