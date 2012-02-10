#include "devfs.h"
#include "common.h"
#include "kmalloc.h"
#include "log.h"
#include "string.h"

struct devfs_inode {
    struct devfs_inode *next;
    char const *name;
    vnode_t *node;
};
typedef struct devfs_inode devfs_inode_t;

static devfs_inode_t *devices = NULL;

static vnode_t root;
static vnodeops_t vnodeops;
static vfsops_t vfsops;

static int devfs_root(vfs_t *vfs, vnode_t *out)
{
    UNUSED_ARGUMENT(vfs);

    out->v_op = root.v_op;
    out->v_data = root.v_data;

    return 0;
}

static int devfs_open(vnode_t *n)
{
    UNUSED_ARGUMENT(n);

    return -1;
}
static int devfs_read(vnode_t *node, void *buf, uint32_t count)
{
    UNUSED_ARGUMENT(node);
    UNUSED_ARGUMENT(buf);
    UNUSED_ARGUMENT(count);

    return -1;
}

static int devfs_write(vnode_t *n, char const *s, size_t l)
{
    UNUSED_ARGUMENT(n);
    UNUSED_ARGUMENT(s);
    UNUSED_ARGUMENT(l);

    return -1;
}

static int devfs_getattr(vnode_t *n, vattr_t *attr)
{
    UNUSED_ARGUMENT(n);

    attr->file_size = 0;

    return 0;
}

static int devfs_lookup(vnode_t *dir, char const *name, vnode_t *out)
{
    UNUSED_ARGUMENT(dir);

    if (name == NULL) {
        log_error("devfs_lookup",
                  "Trying to lookup a null path\n");
        return -1;
    }

    devfs_inode_t *i;
    for (i = devices; i != NULL; i = i->next) {
        if (strcmp(i->name, name) == 0) {
            out->v_op = i->node->v_op;
            out->v_data = i->node->v_data;
            return 0;
        }
    }

    return -1;
}

int devfs_add_device(char const *path, vnode_t *node)
{
    if (path == NULL || strlen(path) == 0) {
        log_error("devfs_add_device",
                  "path is empty or NULL\n");
        return -1;
    }

    devfs_inode_t *i;
    for (i = devices; i != NULL; i = i->next) {
        if (strcmp(i->name, path) == 0) {
            log_error("devfs_add_device",
                      "Trying to add a device that is already added: %s\n",
                      path);
            return -1;
        }
    }

    int len = strlen(path) + 1;
    char *copy = kmalloc(len);
    if (copy == NULL) {
        log_error("devfs_add_device",
                  "Could not allocated memory for path name %s\n",
                  path);
        return -1;
    }
    memcpy(copy, path, len);

    devfs_inode_t *inode = kmalloc(sizeof(devfs_inode_t));
    if (inode == NULL) {
        log_error("devfs_add_device",
                  "Could not allocated memory for devfs_inode_t struct\n");
        kfree(copy);
        return -1;
    }

    inode->next = NULL;
    inode->name = copy;
    inode->node = node;

    if (devices == NULL) {
        devices = inode;
    } else {
        devices->next = inode;
    }

    return 0;
}

int devfs_init(vfs_t *vfs)
{
    vnodeops.vn_open = &devfs_open;
    vnodeops.vn_getattr = &devfs_getattr;
    vnodeops.vn_read = &devfs_read;
    vnodeops.vn_lookup = &devfs_lookup;
    vnodeops.vn_write = &devfs_write;
    root.v_op = &vnodeops;

    vfsops.vfs_root = &devfs_root;
    vfs->vfs_op = &vfsops;
    vfs->vfs_data = 0;

    return 0;
}
