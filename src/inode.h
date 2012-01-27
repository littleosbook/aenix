#ifndef INODE_H
#define INODE_H

#include "stdint.h"

#define FILENAME_MAX_LEN 256

#define FILETYPE_REG 0
#define FILETYPE_DIR 1

struct inode {
    uint32_t type;
    uint32_t location;
    uint32_t size; /* in bytes */
} __attribute__((packed));
typedef struct inode inode_t;

struct direntry {
    char name[FILENAME_MAX_LEN];
    uint32_t location;
} __attribute__((packed));
typedef struct direntry direntry_t;

#endif /* INODE_H */
