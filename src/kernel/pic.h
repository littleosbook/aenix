#ifndef PIC_H
#define PIC_H

#include "stdint.h"

#define PIC1_START      0x20
#define PIC2_START      0x28
#define PIC_NUM_IRQS    16

void pic_init(void);
void pic_acknowledge(void);
void pic_mask(uint8_t mask1, uint8_t mask2);

#endif /* PIC_H */
