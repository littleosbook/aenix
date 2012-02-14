#include "vfs.h"
#include "string.h"
#include "log.h"
#include "kmalloc.h"

static vfs_t *vfs_list = NULL;

int vfs_mount(char const *path, vfs_t *vfs)
{
    if (vfs_list == NULL) {
        vfs_list = vfs;
    } else {
        vfs_t *i = vfs_list;
        while (i->vfs_next != NULL) {
            if (strcmp(i->mount_path, path) == 0) {
                return -1;
            }
            i = i->vfs_next;
        }
        i->vfs_next = vfs;
    }

    int plen = strlen(path) + 1;
    char *p = kmalloc(plen * sizeof(char));
    memcpy(p, path, plen);
    vfs->mount_path = p;
    vfs->vfs_next = NULL;

    return 0;
}

static int find_longest_mount_path(char const *path, vfs_t **out)
{
    vfs_t *i = vfs_list;
    vfs_t *root;
    uint32_t max_len = 0, idx;
    while (i != NULL) {
        char const *mp = i->mount_path;
        char const *p = path;
        for (idx = 0; *p && *mp && *p == *mp; ++p, ++mp, ++idx) {
        }
        if (idx > max_len) {
            root = i;
            max_len = idx;
        }
        i = i->vfs_next;
    }

    *out = root;
    return max_len;

}

int vfs_lookup(char const *path, vnode_t *res)
{
    if (strlen(path) < 1) {
        return -1;
    }

    if (path[0] != '/') {
        return -1;
    }


    vfs_t *root;
    int max_len = find_longest_mount_path(path, &root);
    path += max_len;

    vnode_t vnode;
    if (root->vfs_op->vfs_root(root, &vnode)) {
        log_error("vfs_lookup", "Could not find vnode for path %s\n", path);
        return -1;
    }

    if (strlen(path) == 0) {
        res->v_op = vnode.v_op;
        res->v_data = vnode.v_data;
        return 0;
    }

    int ret = 0, inc;
    uint32_t slash_index;
    char *p = kmalloc(strlen(path) * sizeof(char) + 1);
    char *orgp = p;
    memcpy(p, path, strlen(path) + 1);

    while (strlen(p) != 0 && ret == 0) {
        slash_index = strcspn(p, "/");

        if (slash_index != strlen(p)) {
            p[slash_index] = '\0';
            inc = slash_index + 1;
        } else {
            inc = slash_index;
        }

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

int vfs_write(vnode_t *node, char const *str, size_t len)
{
	return node->v_op->vn_write(node, str, len);
}

int vfs_getattr(vnode_t *node, vattr_t *attr)
{
    return node->v_op->vn_getattr(node, attr);
}
