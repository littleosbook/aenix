# The Road to User Mode

Now that the kernel boots, prints to screen and reads from keyboard - what do
we do? Usually, a kernel is not supposed to do the application logic itself,
but leave that for actual applications. The kernel creates the proper
abstractions (for memory, files, devices, etc.) so that application development
becomes easier, performs tasks on behalf of applications (system calls), and
schedules processes.

User mode, in contrast with kernel mode, is how the user's programs executes.
It is less privileged than the kernel, and will prevent badly written user
programs from messing with other programs or the kernel. Badly written kernels
are free to mess up what they want.

There's quite a way to go until we get there, but here follows a
quick-and-dirty start.

## Loading a program

Where do we get the application from?

TODO:

### GRUB Modules

TODO:

## Executing the program

### A very simple program

Since for now any program we'd write would have trouble to communicate with the
outside, a very short program that writes a value to a register suffices.
Halting Bochs and reading its log should verify that the program has run.

~~~ {.nasm}
    ; set eax to some distinguishable number, to read from the log afterwards
    mov eax, 0xDEADBEEF

    ; enter infinite loop, nothing more to do
    ; $ means "beginning of line", ie. the same instruction
    jmp $
~~~

### Compiling

Since our kernel cannot parse advanced executable formats, we need to compile
the code into a flat binary. NASM can do it like this:

    nasm -f bin program.s -o program

This is all we need.

### Jumping to the code

What we'd like to do now is just jump to the address of the GRUB-loaded module.
Since it is easier to parse the multiboot-structure in C, calling the code from
C is more convenient, but it can of course be done in assembler as well.

~~~ {.c}
    typedef void (*call_module_t)(void);
    /* ... */
    call_module_t start_program = (call_module_t) address_of_module;
    start_program();
    /* we'll never get here */
~~~

If we start the kernel, wait until it has run and entered the infinite loop in
the program, and halt Bochs we should see `0xDEADBEEF` in `eax`. We have
successfully started a program in our OS!

## The beginning of user mode

This is too simplistic...
