# Interrupts and input
Now that OS can produce _output_, it would be nice if it also could get some
_input_. In order to read information from the keyboard, the operating system
must be able to handle _interrupts_. An interrupt occurs when a hardware
device, such as the keyboard, the serial port or the timer, signals the CPU
that the state of the device has changed. The CPU itself can also be send
interrupts due to program errors, for example when a program references memory
it doesn't has access to, or when a program divides a number by zero. Finally,
there are also _software intterupts_, which are interrupts that are cause by
the `INT` assembly instruction, and they are often used for system calls
(syscalls).

## Interrupts handlers
The interrupts are handle via the _interrupt descriptor table_ (IDT). The IDT
that describes a handler for each interrupt. Each interrupt has a number (0 -
255), and the handler for interrupt _i_ is defined at the _i:th_ position in
the vector. There are three different kinds of handlers for interrupts:

- Task handler
- Interrupt handler
- Trap handler

The task handler uses Intel specific functionality, so they won't be covered
here (see the Intel manual [@intel3a], chapter 6, for more info). The only
difference between an interrupt handler and a trap handler is that the
interrupt handler disables interrupts, that is, you can't get an interrupt
while at the same time handling an interrupt. In this book, we will use trap
handlers and disables interrupts manually when we need to do so.

## Creating an interrupt handler
An entry in the IDT for an interrupt handler consists of 64 bits.
The highest 32 bits are

    Bit:     | 31              16 | 15 | 14 13 | 12 | 11 | 10 9 8 | 7 6 5 | 4 3 2 1 0 |
    Content: | offset high        | P  | DPL   | 0  | D  | 1  1 0 | 0 0 0 | reserved  |

and the lowest 32 bits are

    Bit:     | 31              16 | 15              0 |
    Content: | segment selector   | offset low        |

where the content is

             Name Description
----------------- ------------
      offset high The 16 highest bits of the 32 bit address in the segment
       offset low The 16 lowest bits of the 32 bits address in the segment
                p If the handler is present in memory or not (1 = present, 0 = not present)
              DPL Descriptor Privilige Level, the privilege level the handler runs in
                D Size of gate, (1 = 32 bits, 0 = 16 bits)
 segment selector The index in the GDT
                r Reserved

The offset is a pointer to code (preferably an assembly label).

##  Loading the IDT
The IDT is loaded with `LIDT` assembly instruction which takes the address of
the first element in the array.
