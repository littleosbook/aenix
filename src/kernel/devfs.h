#ifndef DEVFS_H
#define DEVFS_H

#include "vfs.h"
#include "vnode.h"

int devfs_init(vfs_t *vfs);
int devfs_add_device(char const *path, vnode_t *node);

#endif /* DEVFS_H */
