#include "fb.h"

#define FB_MEMORY 0xB8000
#define FB_NUM_COLS 80
#define FB_NUM_ROWS 25

#define BLACK_ON_WHITE 0x0F

static uint8_t *fb = (uint8_t *) FB_MEMORY;

void fb_write(uint8_t c, uint32_t row, uint32_t col)
{
    uint8_t *cell = fb + 2 * (row*FB_NUM_COLS + col);
    cell[0] = c;
    cell[1] = BLACK_ON_WHITE;
}

void fb_clear()
{
    uint8_t i, j;
    for (i = 0; i < FB_NUM_ROWS; ++i) {
        for (j = 0; j < FB_NUM_COLS; ++j) {
            fb_write(' ', i, j);
        }
    }
}
