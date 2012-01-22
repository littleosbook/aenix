#include "fb.h"
#include "io.h"

#define FB_MEMORY 0xB8000
#define FB_NUM_COLS 80
#define FB_NUM_ROWS 25

#define FB_CURSOR_DATA_PORT 0x3D5
#define FB_CURSOR_INDEX_PORT 0x3D4

#define FB_HIGH_BYTE 14
#define FB_LOW_BYTE 15

#define BLACK_ON_WHITE 0x0F

#define TO_ADDRESS(row, col) (fb + 2 * (row * FB_NUM_COLS + col))

#define FB_BACKSPACE_ASCII 8

static uint8_t *fb = (uint8_t *) FB_MEMORY;
static uint16_t cursor_pos;

static void write_cell(uint8_t *cell, uint8_t b)
{
    cell[0] = b;
    cell[1] = BLACK_ON_WHITE;
}

static void set_cursor(uint16_t loc)
{
    outb(FB_CURSOR_INDEX_PORT, FB_HIGH_BYTE);
    outb(FB_CURSOR_DATA_PORT, loc >> 8);
    outb(FB_CURSOR_INDEX_PORT, FB_LOW_BYTE);
    outb(FB_CURSOR_DATA_PORT, loc);
}

static void move_cursor_forward(void)
{
    cursor_pos++;
    set_cursor(cursor_pos);
}

static void move_cursor_back(void)
{
    if (cursor_pos != 0) {
        cursor_pos--;
        set_cursor(cursor_pos);
    }
}

static void move_cursor_down()
{
    cursor_pos += FB_NUM_COLS;
    set_cursor(cursor_pos);
}

static void move_cursor_start()
{
    cursor_pos -= cursor_pos % FB_NUM_COLS;
    set_cursor(cursor_pos);
}

static void scroll()
{
    uint32_t r, c;
    for (r = 1; r < FB_NUM_ROWS; ++r) {
        for (c = 0; c < FB_NUM_COLS; ++c) {
            fb_write(fb_read(r, c), r - 1, c);
        }
    }

    for (c = 0; c < FB_NUM_COLS; ++c) {
        fb_write(' ', FB_NUM_ROWS - 1, c);
    }
}

void fb_put_b(uint8_t b)
{
    if (b != '\n' && b != '\t' && b != FB_BACKSPACE_ASCII) {
        uint8_t *cell = fb + 2 * cursor_pos;
        write_cell(cell, b);
    }

    if (b == '\n') {
        move_cursor_down(); 
        move_cursor_start();
    } else if (b == FB_BACKSPACE_ASCII) {
        move_cursor_back();
        uint8_t *cell = fb + 2 * cursor_pos;
        write_cell(cell, ' ');
    } else if (b == '\t') {
        int i;
        for (i = 0; i < 3; ++i) {
            fb_put_b(' ');
        }
    } else {
        move_cursor_forward();
    }

    if (cursor_pos >= FB_NUM_COLS * FB_NUM_ROWS) {
        scroll();
        fb_move_cursor(24, 0);
    }
}

void fb_put_s(char *s)
{
    while (*s != '\0') {
        fb_put_b(*s++);
    }
}

void fb_put_ui(uint32_t i)
{
    /* FIXME: please make this code more beautiful */
    uint32_t n, digit;
    if (i >= 1000000000) {
        n = 1000000000;
    } else {
        n = 1;
        while (n*10 < i) {
            n *= 10;
        }
    }
    while (n > 0) {
        digit = i / n;
        fb_put_b('0'+digit);
        i %= n;
        n /= 10;
    }
}

void fb_put_ui_hex(uint32_t i)
{
    fb_put_ui_hex_pad(i, 0);
}

void fb_put_ui_hex_pad(uint32_t i, uint8_t min_digits)
{
    char *digits = "0123456789ABCDEF";
    uint32_t n, digit;

    /* find the largest nibble to output */
    if (i >= 0xF0000000) {
        n = 28;
    } else {
        n = 0;
        while ((((uint32_t)0x01) << (n+4)) < i) {
            n += 4;
        }
    }

    fb_put_s("0x");

    /* pad with zeroes */
    if (min_digits > 0) {
        min_digits -= 1;
    }
    min_digits <<= 2;
    while (min_digits > n) {
        fb_put_b('0');
        min_digits -= 4;
    }
    /* print the number */
    while (1) {
        digit = (i >> n) & 0x0000000F;
        fb_put_b(digits[digit]);
        if (n == 0) {
            break;
        }
        n -= 4;
    }
}

void fb_write(uint8_t b, uint32_t row, uint32_t col)
{
    uint8_t *cell = TO_ADDRESS(row, col);
    write_cell(cell, b);
}

uint8_t fb_read(uint32_t row, uint32_t col)
{
    uint8_t *cell = TO_ADDRESS(row, col);
    return *cell;
}

void fb_clear()
{
    uint8_t i, j;
    for (i = 0; i < FB_NUM_ROWS; ++i) {
        for (j = 0; j < FB_NUM_COLS; ++j) {
            fb_write(' ', i, j);
        }
    }
    fb_move_cursor(0, 0);
}

void fb_move_cursor(uint16_t row, uint16_t col)
{
    uint16_t loc = row*FB_NUM_COLS + col;
    cursor_pos = loc;
    set_cursor(loc);
}
