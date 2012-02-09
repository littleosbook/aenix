#ifndef VNODE_H
#define VNODE_H

#include "stdint.h"

struct vnodeops;

struct vnode {
    struct vnodeops *v_op;
    uint32_t v_data;
};
typedef struct vnode vnode_t;

struct vnodeops {
    int (*vn_write)(vnode_t *node);
    int (*vn_lookup)(vnode_t *dir, char const *name, vnode_t *res);
};


#endif /* VNODE_H */
