#include "keyboard.h"
#include "io.h"

#define KBD_DATA_PORT 0x60

uint8_t kbd_read(void)
{
	return inb(KBD_DATA_PORT);
}
