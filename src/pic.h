#ifndef PIC_H
#define PIC_H

#include "stdint.h"

void pic_init(void);
void pic_acknowledge(void);
void pic_mask(uint8_t mask1, uint8_t mask2);

#endif /* PIC_H */
