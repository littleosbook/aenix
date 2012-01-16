#ifndef FB_H
#define FB_H

#include "stdint.h"

void fb_write(uint8_t ch, uint32_t row, uint32_t col); 
void fb_clear();

#endif /* FB_H */
