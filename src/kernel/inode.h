#ifndef INODE_H
#define INODE_H

#include "stdint.h"

#define FILENAME_MAX_LEN 254

#define FILETYPE_REG 0
#define FILETYPE_DIR 1

#define BLOCK_SIZE 1024 /* in bytes */
#define INODE_NUM_BLOCKS 5
#define INODE_LIST_NUM_BLOCKS 7

/* sizeof(inode_t) == 16 bytes */
struct inode {
    uint8_t type;
    uint8_t size_high;
    uint16_t size_low; /* in bytes */
    uint16_t blocks[INODE_NUM_BLOCKS];
    uint16_t inode_tail;
} __attribute__((packed));
typedef struct inode inode_t;

/* sizeof(inode_list_t) == 16 bytes */
struct inode_list {
    uint32_t padding;
    uint16_t blocks[INODE_NUM_BLOCKS];
    uint16_t inode_tail;
} __attribute__((packed));
typedef struct inode_list inode_list_t;

/* sizeof(direntry_t) == 256 bytes */
struct direntry {
    char name[FILENAME_MAX_LEN];
    uint16_t inode_id;
} __attribute__((packed));
typedef struct direntry direntry_t;

/* sizeof(superblock_t) == 8 bytes */
struct superblock {
    uint16_t num_inodes;
    char *mount_path;
    uint16_t start_block;
} __attribute__((packed));
typedef struct superblock superblock_t;

#define INODE_SIZE(inode) ((((uint32_t) (inode)->size_high) << 16) | \
                           ((inode)->size_low))
#define DIRENTRES_PER_BLOCK (BLOCK_SIZE/sizeof(direntry_t))

#define INODE_IS_REG(inode) ((inode)->type == FILETYPE_REG)
#define INODE_IS_DIR(inode) ((inode)->type == FILETYPE_DIR)

#endif /* INODE_H */
