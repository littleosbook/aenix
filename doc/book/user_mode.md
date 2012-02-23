# User Mode

TODO:

## Segments for user mode

To enable user mode we need to add two more segments to the GDT. They are very
similar to the kernel segments we added when we [set up the
GDT](#the-global-descriptor-table-gdt) in the [chapter about
segmentation](#segmentation).

  Index   Offset   Name                 Address range             Type   DPL
-------  -------   -------------------  ------------------------- -----  ----
      3   `0x18`   user code segment    `0x00000000 - 0xFFFFFFFF` RX     PL3
      4   `0x20`   user data segment    `0x00000000 - 0xFFFFFFFF` RW     PL3

Table: Table 6-1: The segment descriptors needed for user mode.

## Setting up for user mode

TODO:

- allocate two page frames, one for code, one for stack
- copy code to code pf
- set up a PDT with PT's. map in the code and stack

## Entering user mode

The only way to execute code with a lower privilege level than the current
privilege level (CPL) is to execute an `iret` or `lret` instruction - interrupt
return or long return, respectively.

To enter user mode, we set up the stack as if the processor had raised an
inter-level interrupt. The stack should look like this:

    [esp + 16]  ss      ; the stack segment selector we want for user mode
    [esp + 12]  esp     ; the user mode stack pointer
    [esp +  8]  eflags  ; the control flags for user mode
    [esp +  4]  cs      ; the code segment selector
    [esp +  0]  eip     ; the instruction pointer of user mode code to execute

(source: The Intel manual [@intel3a], section 6.2.1, figure 6-4)

Before we execute `iret` we need to change to the page directory we setup for
the user mode process. Important to remember here is that, to continue executing
kernel code after we've switched PDT, the kernel needs to be mapped in. One way
to accomplish this is to have a "kernel PDT", which maps in all kernel data at
`0xC0000000` and above, and merge it with the user PDT (which only maps below
`0xC0000000`) when we set it.

TODO: talk about `eflags`

The `eip` should point to the entry point for the user code - `0x00000000` in
our case. `esp` should be where the stack should start - `0xBFFFFFFB`.

`cs` and `ss` should be the segment selectors for the user code and user data
segments, respectively. As we saw in the [segmentation
chapter](#creating-and-loading-the-gdt), the lowest two bits of a segment
selector is the RPL - the Requested Privilege Level. When we execute the `iret` we
want to enter PL3, so the RPL of `cs` and `ss` should be 3.

    cs = 0x18 | 0x3
    ss = 0x20 | 0x3

Now we are ready to execute `iret`. If everything has been set up right, we now
have a kernel that can enter user mode. Yay!

## Using C for user mode programs

The kernel is compiled as an ELF [@wiki:elf] binary, which is a format that
allows for more complex and convenient layouts and functionality than a plain
flat binary. The reason we can use ELF for the kernel is that GRUB knows how to
parse the ELF structure.

If we implement an ELF parser, we can compile the user mode programs into ELF
binaries as well. We leave this as an exercise for the reader.

One thing we can do to make it easier to develop user mode programs is to allow
them to be written in C (but still compile them to flat binaries). In C, the
layout of the generated code is more unpredictable and the entry point
(`main()`) might not be at offset 0.

One way to solve this is to add a few assembler lines is placed at offset 0,
and which calls `main()`.

Assembler code (`start.s`):

~~~ {.nasm}
extern main

section .text
    ; push argv
    ; push argc
    call main
    ; main has returned, eax is return value
    jmp  $    ; loop forever
~~~

Linker script (`link.ld`) to place it first:

    OUTPUT_FORMAT("binary")

    SECTIONS
    {
        . = 0;

        .text ALIGN(4):
        {
            start.o(.text)
            *(.text)
        }

        .data ALIGN(4):
        {
            *(.data)
        }

        .rodata ALIGN(4):
        {
            *(.rodata*)
        }
    }

Note: `*(.text)` will not include the `.text` section of `start.o` again. See
[@ldcmdlang] for more details.

With this script we can write programs in C or assembler (or any other language
that compiles to object files linkable with `ld`), and it is easy to load and
map in for the kernel. (.rodata will not be mapped in as read-only, though.)

We want the usual `CFLAGS`:

    -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles
    -nodefaultlibs

And these `LDFLAGS`:

    -T link.ld -melf_i386

### Lib C

It might now be interesting to start thinking about writing a short libc for
your programs. Some of the functionality requires system calls (?????) to work,
but some, such as the `string.h` functions, does not.

## Further reading

TODO: 
