# Page frame allocation

Now that we have virtual memory with paging set up, we need to somehow allocate
some of the available memory to use for the applications we want to run. How do
we know what memory is free? That is the role of a page frame allocator, and
we'll .....

## Managing available memory

### How much memory is there?

First we need to know how much memory the computer we're running on has.
Perhaps the easiest way to do this is to read it from the multiboot [@multiboot]
structure sent to us by GRUB. This will give us the information we need about
the memory - what is reserved, I/O mapped, read-only etc. The rest is free to
use. We must make sure that we don't mark the part of memory used by the kernel
as free. An easy way to do this is to export labels at the beginning and end of
the kernel binary from the linker script.

The updated linker script:

    ENTRY(loader)           /* the name of the entry symbol */

    . = 0xC0100000          /* the code should be relocated to 3GB + 1MB */

    /* these labels get exported to the code files */
    kernel_virtual_start = .;
    kernel_physical_start = . - 0xC0000000;

    /* align at 4KB and load at 1MB */
    .text ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(.text)            /* all text sections from all files */
    }

    /* align at 4KB and load at 1MB + . */
    .rodata ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(.rodata*)         /* all read-only data sections from all files */
    }

    /* align at 4KB and load at 1MB + . */
    .data ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(.data)            /* all data sections from all files */
    }

    /* align at 4KB and load at 1MB + . */
    .bss ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(COMMON)           /* all COMMON sections from all files */
        *(.bss)             /* all bss sections from all files */
    }

    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;

NASM code can now read these labels directly, and perhaps push them onto the
stack so we can use them from C:

~~~ {.nasm}
extern kernel_virtual_start
extern kernel_virtual_end
extern kernel_physical_start
extern kernel_physical_end

; ...

push kernel_physical_end
push kernel_physical_start
push kernel_virtual_end
push kernel_virtual_start

call some_function
~~~

There is no clean way to take the address of a label directly from C. One way
to do it is to declare the label as a function and take the address of the
function:

~~~ {.c}
void kernel_virtual_start(void);

/* ... */

unsigned int vaddr = (unsigned int) &kernel_virtual_start;
~~~

If we use GRUB modules we also need to make sure they are marked as "in use" as
well.

Note that not all available memory needs to be contiguous. In the first 1MB
there are several I/O-mapped memory sections, as well as memory used by GRUB
and the BIOS. Other parts of the memory might be similarly unavailable.

The memory sections need also be divided up into complete page frames. This is
easy to do by just .....

### Free lists...

TODO:
Lists, bitmaps, etc...

## How can we access a page frame?

TODO: temporarily map it in...

## kmalloc()

TODO: 

## Further reading

TODO: there should be quite a lot to link to...
