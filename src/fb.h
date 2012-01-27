#ifndef FB_H
#define FB_H

#include "stdint.h"

void fb_put_b(uint8_t b);
void fb_put_s(char *s);

/* output unsigned integer in decimal format */
void fb_put_ui(uint32_t i);
/* output unsigned integer in hexadecimal format */
void fb_put_ui_hex(uint32_t i);
/* output unsigned integer in hexadecimal format, pad if necessary */
void fb_put_ui_hex_pad(uint32_t i, uint8_t min_digits);

void fb_write(uint8_t ch, uint32_t row, uint32_t col);
uint8_t fb_read(uint32_t row, uint32_t col);
void fb_clear();
void fb_move_cursor(uint16_t row, uint16_t col);

#endif /* FB_H */
