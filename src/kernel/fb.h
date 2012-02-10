#ifndef FB_H
#define FB_H

#include "stdint.h"
#include "vnode.h"

void fb_put_b(uint8_t b);
void fb_put_s(char const *s);

/* output unsigned integer in decimal format */
void fb_put_ui(uint32_t i);
/* output unsigned integer in hexadecimal format */
void fb_put_ui_hex(uint32_t i);

void fb_clear();
void fb_move_cursor(uint16_t row, uint16_t col);

int fb_init(void);
int fb_get_vnode(vnode_t *out);

#endif /* FB_H */
