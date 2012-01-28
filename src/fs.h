#ifndef FS_H
#define FS_H

#include "inode.h"
#include "stdint.h"

void fs_init(uint32_t root_addr);
inode_t *fs_find_inode(char *path);
uint32_t fs_get_addr(inode_t *node);

#endif /* FS_H */
