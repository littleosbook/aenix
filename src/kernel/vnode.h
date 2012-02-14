#ifndef VNODE_H
#define VNODE_H

#include "stddef.h"
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
    int (*vn_read)(vnode_t *node, void *buf, size_t count);
    int (*vn_write)(vnode_t *node, char const *buf, size_t count);
    int (*vn_getattr)(vnode_t *node, vattr_t *attr);
};
typedef struct vnodeops vnodeops_t;

/* NOTE: This is not a "deep" copy. The v_op pointer will point to the
 * same struct in both from and to.
 */
void vnode_copy(vnode_t *from, vnode_t *to);

#endif /* VNODE_H */
