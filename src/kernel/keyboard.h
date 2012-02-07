#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"

uint32_t kbd_init(void);
uint8_t kbd_read_scan_code(void);
uint8_t kbd_scan_code_to_ascii(uint8_t scan_code);

#endif /* KEYBOARD_H */
