#ifndef FS_H
#define FS_H

#include "inode.h"
#include "stdint.h"

uint32_t fs_init(uint32_t fs_paddr, uint32_t fs_size);
inode_t *fs_find_inode(char const *path);
uint32_t fs_get_addr(inode_t *node);

#endif /* FS_H */
