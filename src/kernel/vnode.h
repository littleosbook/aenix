#ifndef VNODE_H
#define VNODE_H

#include "stdint.h"
#include "vattr.h"

struct vnodeops;

struct vnode {
    struct vnodeops *v_op;
    uint32_t v_data;
};
typedef struct vnode vnode_t;

struct vnodeops {
    int (*vn_open)(vnode_t *node);
    int (*vn_lookup)(vnode_t *dir, char const *name, vnode_t *res);
    int (*vn_read)(vnode_t *node, void *buf, uint32_t count);
    int (*vn_getattr)(vnode_t *node, vattr_t *attr);
};
typedef struct vnodeops vnodeops_t;

#endif /* VNODE_H */
