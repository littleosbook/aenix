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
must be used to communicate with the device. `outb` takes two parameters, the
address of the device and the value to send to the device. `inb` takes a single
parameter, simply address of the device. One can think of I/O ports as
communicating with the hardware the same as one communicate with a server using
sockets. For example, the cursor (the blinking rectangle) of the framebuffer is
controlled via I/O ports.

### The framebuffer
The framebuffer is a hardware device that is capable of displaying a buffer of
memory on the screen [@wiki:fb].
