#include "vnode.h"

void vnode_copy(vnode_t *from, vnode_t *to)
{
    to->v_op = from->v_op;
    to->v_data = from->v_data;
}
