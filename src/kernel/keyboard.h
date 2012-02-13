#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"
#include "vnode.h"

uint32_t kbd_init(void);
int kbd_get_vnode(vnode_t *out);

#endif /* KEYBOARD_H */
