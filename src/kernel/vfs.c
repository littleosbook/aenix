#include "vfs.h"
#include "string.h"
#include "log.h"
#include "kmalloc.h"

static vfs_t *vfs;

int vfs_lookup(char const *path, vnode_t *res)
{
    if (strlen(path) < 1) {
        return -1;
    }

    if (path[0] != '/') {
        return -1;
    }

    vnode_t vnode;
    if (vfs->vfs_op->vfs_root(vfs, &vnode)) {
        log_error("vfs_lookup",
                  "Can't find root vnode for path %s\n",
                  path);
        return -1;
    }

    int ret = 0, inc;
    uint32_t slash_index;
    char *p = kmalloc(strlen(path) * sizeof(char) + 1);
    char *orgp = p;
    memcpy(p, path, strlen(path) + 1);

    ++p; /* skip the first / */
    while (strlen(p) != 0 && ret == 0) {
        slash_index = strcspn(p, "/");

        if (slash_index != strlen(p)) {
            p[slash_index] = '\0';
            inc = slash_index + 1;
        } else {
            inc = slash_index;
        }

        log_debug("vfs_lookup", "p: %s\n", p);

        ret = vnode.v_op->vn_lookup(&vnode, p, res);
        vnode.v_op = res->v_op;
        vnode.v_data = res->v_data;

        p += inc;
    }

    kfree(orgp);

    return ret;
}

int vfs_open(vnode_t *node)
{
    return node->v_op->vn_open(node);
}

int vfs_read(vnode_t *node, void *buf, uint32_t count)
{
    return node->v_op->vn_read(node, buf, count);
}

int vfs_getattr(vnode_t *node, vattr_t *attr)
{
    return node->v_op->vn_getattr(node, attr);
}

int vfs_init(vfs_t *root_vfs)
{
    vfs = root_vfs;
    return 0;
}
