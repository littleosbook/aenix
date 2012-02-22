# Input and output (I/O)

After booting our very small operating system kernel, the next task will be to
be able to display text on the console. In this chapter we will create our
first _drivers_, that is, code that acts as an layer between our kernel and the
hardware, providing a nicer abstraction than speaking directly to the hardware.
The first part of this chapter creates a driver for the framebuffer
[@wiki:fb] to be able to display text to the user. The second part will
create a driver for the serial port, since BOCHS can store output from the
serial port in a file, effectively creating a logging mechanism for our
operating system!

## Different kinds of I/O
There are usually two different ways to "talk" to the I/O hardware,
_memory-mapped I/O_ and _I/O ports_.

If the hardware uses memory-mapped I/O, then one
can write to a specific memory address and the hardware will be automatically
get the new values. One example of this is the framebuffer, which will be
discussed more in detail later. If you write the ASCII [@wiki:ascii] the value
`0x410F` to address `0x000B8000`, you will see the letter A in white color on a
black background (see the section [The framebuffer](#the-framebuffer) for more
details).

If the hardware uses I/O ports, then the assembly instructions `outb` and `inb`
must be used to communicate with the device. `out` takes two parameters, the
address of the device and the value to send to the device. `in` takes a single
parameter, simply address of the device. One can think of I/O ports as
communicating with the hardware the same as one communicate with a server using
sockets. For example, the cursor (the blinking rectangle) of the framebuffer is
controlled via I/O ports.

### The framebuffer
The framebuffer is a hardware device that is capable of displaying a buffer of
memory on the screen [@wiki:fb]. The framebuffer has 80 columns and 25 rows,
and their indices start at 0 (so rows are labelled 0 - 24).

### Writing text
Writing text to the console via the framebuffer is done by
memory-mapped I/O. The starting address of memory-mapped I/O for the
framebuffer is `0x000B8000`. The memory is divided into 16 bit cells, where the
16 bit determines both the character, the foreground color and the background
color. The highest 8 bits determines the character, bit 7 - 3 the background
and bit 3 - 0 the foreground, as can be seen in the following figure:

    Bit:     | 15 14 13 12 11 10 9 8 | 7 6 5 4 | 3 2 1 0 |
    Content: | ASCII                 | FG      | BG      |

The available colors are:

 Color Value       Color Value         Color Value          Color Value
------ ------ ---------- ------- ----------- ------ ------------- ------
 Black 0             Red 4         Dark grey 8          Light red 12
  Blue 1         Magenta 5        Light blue 9      Light magenta 13
 Green 2           Brown 6       Light green 10       Light brown 14
  Cyan 3      Light grey 7        Light cyan 11             White 15

The first cell corresponds to row zero, column zero on the console.  Using the
ASCII table, one can see that A corresponds to 65 or `0x41`. Therefore, to
write the character A with a green foreground (2) and dark grey background (8),
the following assembly instruction is used

    mov [0x000B8000], 0x4128

The second cell then corresponds to row zero, column one and it's address is

    0x000B8000 + 16 = 0x000B8010

This can all be done a lot easier in C by treating the address `0x000B8000` as
a char pointer, `char *fb = (char *) 0x000B8000`. Then, writing A to at place
(0,0) with green foreground and dark grey background becomes:

    fb[0] = 'A';
    fb[1] = 0x28;

This can of course be wrapped into a nice function

~~~ {.c}
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    fb[i] = c;
    fb[i + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F)
}
~~~

which can then be called

~~~ {.c}
#define FB_GREEN     2
#define FB_DARK_GREY 8

fb_write_cell(0, 'A', FB_GREEN, FB_DARK_GREY);
~~~

### Moving the cursor

Moving the cursor of the framebuffer is done via two different I/O ports.
The cursor position is determined via a 16 bits integer, 0 means row zero,
column zero, 1 means row zero, column one, 80 means row one, column zero and so
on. Since the position is 16 bits large, and the `out` assembly instruction
only take 8 bits as data, the position must be sent in two turns, first 8 bits
then the next 8 bits. The has two I/O ports, one for accepting the data, and
one for describing the data being received. Port `0x3D4` is the command port
that describes the data and port `0x3D5` is for the data.

To set the cursor at row one, column zero (position `80 = 0x0050`), one would
use the following assembly instructions

~~~ {.nasm}
out 0x3D4, 14      ; 14 tells the framebuffer that we will now send the highest 8 bits
out 0x3D5, 0x00    ; sending the highest 8 bits of 0x0050
out 0x3D4, 15      ; 15 tells the framebuffer that we will now send the lowest 8 bits
out 0x3D5, 0x50    ; sending the lowest 8 bits of 0x0050
~~~

The `out` assembly instruction can't be done in C, therefore it's a good idea
to wrap it an assembly function which can be accessed from C via the cdecl
calling standard:

~~~ {.nasm}
global outb             ; make the label outb visible outside this file

; outb - send a byte to (out byte) to an I/O port
; stack: [esp + 8] the data byte
;        [esp + 4] the I/O port
;        [esp    ] return address
outb:
    mov al, [esp + 8]    ; move the data to be sent into the al register
    mov dx, [esp + 4]    ; move the address of the I/O port into the dx register
    out dx, al           ; send the data to the I/O port
    ret                  ; return to the calling function
~~~

By storing this function in a file called `io.s` and also creating a header
`io.h`, the `out` assembly instruction can now be conveniently accessed from C:

~~~ {.c}
#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

/** outb:
 *  Sends the given data to the given I/O port. Defined in io.s
 *
 *  @param port The I/O port to send the data to
 *  @param data The data to send to the I/O port
 */
void outb(unsigned short port, unsigned char data);

#endif /* INCLUDE_IO_H */
~~~

Moving the cursor can now be wrapped in a C function:

~~~ {.c}
#include "io.h"

/* The I/O ports */
#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

void fb_move_cursor(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT,    ((pos >> 8) & 0x00FF));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT,    pos & 0x00FF);
}
~~~

### The driver
HELINO_RESUME
