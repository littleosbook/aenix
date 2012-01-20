#include "keyboard.h"
#include "io.h"
#include "fb.h"

#define KBD_DATA_PORT   0x60

/* Alphabet */
#define KBD_SC_A        0x1e
#define KBD_SC_B        0x30
#define KBD_SC_C        0x2e
#define KBD_SC_D        0x20
#define KBD_SC_E        0x12
#define KBD_SC_F        0x21
#define KBD_SC_G        0x22
#define KBD_SC_H        0x23
#define KBD_SC_I        0x17
#define KBD_SC_J        0x24
#define KBD_SC_K        0x25
#define KBD_SC_L        0x26
#define KBD_SC_M        0x32
#define KBD_SC_N        0x31
#define KBD_SC_O        0x18
#define KBD_SC_P        0x19
#define KBD_SC_Q        0x10
#define KBD_SC_R        0x13
#define KBD_SC_S        0x1f
#define KBD_SC_T        0x14
#define KBD_SC_U        0x16
#define KBD_SC_V        0x2f
#define KBD_SC_W        0x11
#define KBD_SC_X        0x2d
#define KBD_SC_Y        0x15
#define KBD_SC_Z        0x2c

/* Special keys */
#define KBD_SC_ENTER    0x1c
#define KBD_SC_SPACE    0x39
#define KBD_SC_BS       0x0e

uint8_t kbd_read_scan_code(void)
{
    return inb(KBD_DATA_PORT);
}

uint8_t kbd_scan_code_to_ascii(uint8_t scan_code)
{
    if (scan_code & 0x80) {
        /* key release, don't do anything */
        return 0;
    }

    switch (scan_code) {
        case KBD_SC_A:
            return 'a';
        case KBD_SC_B:
            return 'b';
        case KBD_SC_C:
            return 'c';
        case KBD_SC_D:
            return 'd';
        case KBD_SC_E:
            return 'e';
        case KBD_SC_F:
            return 'f';
        case KBD_SC_G:
            return 'g';
        case KBD_SC_H:
            return 'h';
        case KBD_SC_I:
            return 'i';
        case KBD_SC_J:
            return 'j';
        case KBD_SC_K:
            return 'k';
        case KBD_SC_L:
            return 'l';
        case KBD_SC_M:
            return 'm';
        case KBD_SC_N:
            return 'n';
        case KBD_SC_O:
            return 'o';
        case KBD_SC_P:
            return 'p';
        case KBD_SC_Q:
            return 'q';
        case KBD_SC_R:
            return 'r';
        case KBD_SC_S:
            return 's';
        case KBD_SC_T:
            return 't';
        case KBD_SC_U:
            return 'u';
        case KBD_SC_V:
            return 'v';
        case KBD_SC_W:
            return 'w';
        case KBD_SC_X:
            return 'x';
        case KBD_SC_Y:
            return 'y';
        case KBD_SC_Z:
            return 'z';
        case KBD_SC_ENTER:
            return '\n';
        case KBD_SC_SPACE:
            return ' ';
        case KBD_SC_BS:
            return 8;
        default:
            return 0;
    }
}
