#ifndef VFS_H
#define VFS_H

#include "stdint.h"
#include "vnode.h"
#include "vattr.h"

struct vfsops;

struct vfs {
    struct vfs *vfs_next;
    struct vfsops *vfs_op;
    char const *mount_path;
    uint32_t vfs_data;
};
typedef struct vfs vfs_t;

struct vfsops {
    int (*vfs_root)(vfs_t *vfs, vnode_t *root);
};
typedef struct vfsops vfsops_t;

int vfs_mount(char const *path, vfs_t *vfs);
int vfs_lookup(char const *path, vnode_t *res);
int vfs_open(vnode_t *node);
int vfs_read(vnode_t *node, void *buf, uint32_t count);
int vfs_write(vnode_t *node, char const *str, size_t len);
int vfs_getattr(vnode_t *node, vattr_t *attr);

#endif /* VFS_H */
