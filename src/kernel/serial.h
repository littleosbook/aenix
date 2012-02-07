#ifndef SERIAL_H
#define SERIAL_H

#include "stdint.h"

#define COM1 0x3F8
#define COM2 0x2F8

void serial_init(uint16_t com);
void serial_write(uint16_t com, uint8_t data);
uint8_t serial_read(uint16_t com);

#endif /* SERIAL_H */
