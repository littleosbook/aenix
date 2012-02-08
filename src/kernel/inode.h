#ifndef INODE_H
#define INODE_H

#include "stdint.h"

#define FILENAME_MAX_LEN 240

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
    uint16_t inode;
} __attribute__((packed));
typedef struct direntry direntry_t;

/* sizeof(superblock_t) == 8 bytes */
struct superblock {
    uint16_t num_inodes;
    char *mount_path;
    uint16_t padding;
} __attribute__((packed));
typedef struct superblock superblock_t;

#endif /* INODE_H */
