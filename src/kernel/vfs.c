#include "vfs.h"
#include "common.h"

int vfs_lookup(char const *name, vnode_t *res)
{
    UNUSED_ARGUMENT(name);
    UNUSED_ARGUMENT(res);

    return -1;
}

int vfs_open(vnode_t *node)
{
    UNUSED_ARGUMENT(node);

    return -1;
}

int vfs_read(vnode_t *node, void *buf, uint32_t count)
{
    UNUSED_ARGUMENT(node);
    UNUSED_ARGUMENT(buf);
    UNUSED_ARGUMENT(count);

    return -1;
}

int vfs_getattr(vnode_t *node, vattr_t *attr)
{
    UNUSED_ARGUMENT(node);
    UNUSED_ARGUMENT(attr);

    return -1;
}

int vfs_init(vfs_t *root_vfs)
{
    UNUSED_ARGUMENT(root_vfs);

    return -1;
}
