#include "fb.h"

#define FB_MEMORY 0xB8000
#define FB_COL_WIDTH 80

#define BLACK_ON_WHITE 0x0F

static uint8_t *fb = (uint8_t *) FB_MEMORY;

void write(uint8_t c, uint32_t row, uint32_t col)
{
    uint8_t *cell = fb + 2 * (row*FB_COL_WIDTH + col);
    cell[0] = c;
    cell[1] = BLACK_ON_WHITE;
}
