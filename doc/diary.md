% Developer diary
% Erik Helin, Adam Renberg
% 2012-01-27

# 2012-01-16

## Achievements

- wrote a short guide for how to install bochs on Ubuntu with GDB support
- installed nasm
- set up an initial develop environment consisting of:
    - gcc as compiler
    - make as build system
    - nasm as assembler
    - bochs as simulator
    - bash as scripting language
- managed to boot the kernel in bochs using the following two guides:
    - <http://www.jamesmolloy.co.uk/tutorial_html/1.-Environment%20setup.html>
    - <http://wiki.osdev.org/Bare_bones>
- started working on a framebuffer device driver, wrote the two functions
  `fb_write` and `fb_clear`
- started investigating on how to improve the booting of the kernel

## Issues

- the current boot sequence must automatize how grub executes the kernel
- the media used for the OS should probably be an ISO image instead of a
  floppy image if we ever want to boot the OS on a real computer


# 2012-01-17

## Achievements

- read up on x86 assembly, nasm, cdecl, and figured out how to call assembler
  code from c
- wrote short functions to read and write to io ports
- wrote more on the framebuffer device driver, perhaps completed it
    - can move the cursor
    - can print chars at cursor position (`putb`) and print a string (puts)
    - newlines are handled, and scrolling
- wrote a new floppy-creator script (`src/create_image.sh`) that makes it easier
  to start the os - just type make run (no need to tell grub start block or
  size any more
- started to read up on segmentation and gdt/idt stuff

## Issues

- the current floppy-build script requires `sudo` (to mount and use loopback
  devices), and it might cause nautilus (on ubuntu) to open the mounted
  directory, and complain when it is unmounted (can be turned off though)

# 2012-01-18

## Achievements

- the gdt loads correctly. yay!
- wrote code to remap pic interrupts into the idt
- started to write code to initialize the idt

## Issues

- the documentation on the x86 architecture is sparse at best - pic i/o port
  address, especially. We take the values from a tutorial

# 2012-01-19

## Achievements

- interrupts is finally working correctly. This took a while to implement,
  mostly due to having to deal with the PIC and the keyboard.
- added code for printing integers
- improved the build scripts slightly
- joined the `#osdev` irc channel on `irc.freenode.org` where we got help with
  some keyboard hardware details. we also got help on the os-dev forum

## Issues

- We are still having problems with the lack of documentation for various
  hardware, these are the issues that takes the longest time to solve, since we 
  often have to ask for help on irc or os-dev

# 2012-01-20

## Achievements

- the keyboard driver is more or less finished (all normal keys are covered,
  but support for numpad, F1 - F12, Insert, Home etc are missing). We will add
  support for them later on if needed.
- the PIT (Programmable Interrupt Timer) driver seems to be finished
  (hard to test), at least the timer
  fires at different intervals depending on the frequency you set via the
  driver.
- started reading up on paging. Also implemented full support for GRUB
  multiboot info. We can now find reserved and available memory areas, which
  is crucial to set up the page table.

## Issues

- Testing things such as timers will be hard. They seems to work correctly, but 
  it is hard to know if they fire at the exact correct interval.
- We will have to implement a function that print integers in hexadecimal
  notation to ease debugging.

## Notes

- Since the basic infrastructure is now working (console, keyboard, timer),
  the road ahead is not as clear any longer. Now we will probably have to read
  a bit more theory and also start to discuss how we want to design the kernel,
  since at the moment we do not need any more low-level plumbing.

# 2012-01-21

## Achievements

- wrote a function that prints integers in hexadecimal notation

# 2012-01-22

## Achievements

- wrote a simple `printf()` based on the one in K&R
- added configuration file for doxygen
- wrote some doxygen documentation

## Issues

- We need to document some more :D

# 2012-01-23

## Achievements

- wrote and enabled identity (1:1) paging for the first 4MB of memory (this
  is where the kernel resides)
- rewrote the paging so that the kernel is an "upper-half" ("higher-half")
  kernel, which means that memory from `0xC0000000 - 0XFFFFFFFF` is mapped to
  the kernel (the kernel is still at the physical memory at 1MB)

## Issues

- We need to write more documentation on the paging and upper-half kernel
  stuff.

# 2012-01-24

## Achievements

- wrote a script to generate a bootable ISO file with GRUB 2
- rewrote the script to generate a bootable ISO file with GRUB 1 and possibly
  arbitrary binary modules that the kernel load into memory
- we now only depend on `genisoimage` to create the bootable media
- managed to jump to a loaded module and perform an INT 0x86 instruction to
  trigger a sys call interrupt
- make can now automatically build all modules and copy them to the iso

## Issues

- We were not able to jump back to the kernel code after jumping loaded binary
  code. This will require more investigation.
- The document about paging and kernel is still not written, Erik will write it 
  tomorrow morning.

# 2012-01-25

## Achievements

- wrote text about paging, the kernel, and putting the kernel in the upper half
  of memory
- configured pandoc/markdown2pdf to generate html and pdf from the markdown
  files
- changed the interrupt number for syscalls to `0xAE`
- we can call modules loaded by GRUB with call, and they can return to the
  kernel with ret
- determined to change the kernel stack to the end of the kernels 4MB page.
  Started to write code to relocate the modules loaded by GRUB to make it
  safe to put the stack there. The code written today creates an identity
  mapping of all memory (except for the kernels upper-half place), and uses a
  small temporary stack in `.bss`. No module moving as of yet.

# 2012-01-26

## Achievements

- finished the code that moves the modules loaded by GRUB to a well-known
  position
- ported the malloc and free implementations from K&R to aenix. Also set up a
  dedicated heap for the kernel in the first 4 MB.
- created a "real" stack for the kernel, the stack now grows from the end of
  the kernel page towards "lower" addresses. The heap grows from the end of the
  kernel towards the stack.
- created a new entry in the stack page descritor table for the modules.
- discussed how to enable user mode processes and how to layout the memory for
  the processes.
- started to read up on how to change the to code with PL3 instead of PL0
- wrote a driver for the COM ports (only COM1 enabled for now)
- wrote a log module. Since bochs can redirect the output from the COM ports to
  file, we can now log arbitrary text to files (very helpful!)

## Issues

None, as we learn more and more about how everything works together, the issues
are more architectural and less "low-level", which is a nice change!

# 2012-01-27

## Achievements

- wrote the utility `mkfs` for creating a filesystem on a disk. Basically,
  `mkfs` traverses a folder, then writes every file and directory to a
  combined large file along with some headers. The GRUB loads the file as a
  module into memory. Then, the kernel can read the "filesystem" with the help
  of the headers. This is essentially a very small initrd filesystem.
- wrote the very small program `init.s` that just loops forever and placed in
  the `/bin/` folder in the filesystem.
- wrote code the locate the `init` program in the memory.
- managed to jump to the `init` and at the same change the privilege level to
  3. This means that the code in `init` is executing in user mode!

## Issues

- Got a taste today of what it's like to debug kernel code. When changing
  privilege level, the CPU throw fault 13 (General Protection fault). The bug
  was caused by the size of the GDT being specified as 3 when it should be 5
  (we simply forgot to change the size when adding to new entries). This shows
  how important it is to review _all_ kernel code to avoid these mistakes.

# 2012-01-28

## Achievements

- Wrote skeleton code for process creation, planned the structure of how to do
  it.


# 2012-01-30

## Achievements

- Worked on setting up paging tables and such for user mode processes - mapping
  and unmapping memory from virtual to physical.
- Sketched an outline of how to implement the page frame allocator.

## Issues

- Cannot allocate 4kB aligned memory needed for both the process' sections and
  for their page tables. Writing the page frame allocator tomorrow.


# 2012-01-31

## Achievements

- The kernel is now mapped in into virtual memory in 4kB page frames, which
  leaves more room for other kernel data and/or user processes. This breaks
  kmalloc() and the in-memory file system
- Thought a lot on the page frame allocator. Since the page frames for the page
  tables might not actually have a virtual memory address we can't access them
  until we map them in. We implemented a way to temporarily map in a page frame.

## Issues

- kmalloc() and the file system code needs to be updated.

## Notes

- The kernel's code and rodata should be mapped as read-only. This requires the
  kernel to be mapped in as several 4kB pages, the sections to be properly
  aligned (which they are), and the setting-up to be done in assembler.


# 2012-02-01

## Achievements

- Cleaned up some code. Wrote a script to turn c headers with constant
  definitions into .inc files to be included in nasm code.
- Met with Torbjörn
- Started to implement the page frame allocator

## Issues

- We couldn't get the Makefile to correctly use .inc as dependencies for the
  assembler files. We worked around it, but it would be nice for it to work
  directly from the Makefile.


# 2012-02-02

## Achievements

- Wrote a lot of code to do error checking, which caused us to find several
  bugs. `log_err()`, `log_info()` and `log_debug()` were created to simplify
  logging
- "Finished" the page frame allocator.
- Debugged. A lot. Found several bugs, which we fixed.
- Modified `kmalloc()` to use the page frame allocator to ask for page frame-sized
  blocks.
- `process_create()` creates a new process, with code loaded from file and pdt
  set up.
- Can enter user mode for the init program, which is created through
  `process_create()`.


# 2012-02-06

## Achievements

- User mode works, after a long fight with weird and difficult-to-find bugs.
  (The kernel stack pointer in the TSS was pointed at the wrong end, which only
  mattered if the TSS struct was non-static, because then the TSS was located
  next to the stack and overwritten on some interrupts...)
- Syscalls work!
- Restructured code base to simplfy adding "apps" such as init.
- User mode programs can now be written in C. With a modest start on our libc
  (there is an interrupt.h|s and a start.s that is linked first into the
  binary with link.ld), init runs compiled from C.


# 2012-02-07 - 2012-02-10

## Achievements

- AEFS2 - We extended the file system created by mkfs so that it is more general
  and can handle writes. It the "device" is divied into blocks of 1024 bits; the
  first block is a superblock with general info, then there are some inode
  blocks, and then datablocks.
- VFS - We have implemented a virtual file system layer. This deals with the
  path hierarchy in the file systems (name/path lookup) and with mounting file
  systems on top of it. An in-memory AEFS file system is mounted at / and a
  devfs file system is mounted at /dev/. Actually reading a "file" (vnode) is
  delegated to the file system within which it is located.
- devfs - devfs is a quite cool idea and a very simple file system. A devfs is
  mouned at /dev/ and devices (such as the frame buffer, keyboard, serial port
  etc. ) can register vnodes with the devfs. These vnodes (/dev/cons,
  /dev/keyboard, etc.) will then be presented as files to user code. Opening,
  reading, writing etc. to these files will actually be delegated, via the
  VFS, to the corresponding device drivers to do the work. This also means that
  there can actually be hierarchies within these devices, such as /dev/cons/1
  and /dev/cons/2 for two different consoles. These hierarchies will be
  accessed through the file system but managed by the devices.
- `SYS_write` and `SYS_open` work, with file descriptors and the whole chebang.

# 2012-02-13

## Achievements
- Implemented the `SYS_read` system call which enables reading from the
  keyboard, which is mounted under `/dev/keyboard`. However, we haven't
  implemented the ECHO feature that most consoles have (if you type something,
  it will show up on your console, whether the program reads it or not).
- Implemented the `SYS_execve` as a first step towards a multiprocess OS. As a
  result of this, `/bin/init` is now launching `/bin/sh`.
- Wrote our first concurrent code in the kernel when buffering the keyboard
  input.

## Issues
- We were a little bit puzzled about how to handle the input buffering from the
  keyboard, but we settled on the way MINIX and FreeBSD does when the buffer is
  full: discarding the new input.
- We haven't implemented the ECHO feature of the console yet, since we want to
  do it in a proper way (NOT in the keyboard interrupt handler!). To do this,
  we think we want some kind of higher level event handling facility. But in
  order to achieve that, we need kernel processes, so therefore we have started
  to work on the scheduler.
