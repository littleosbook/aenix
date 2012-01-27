2012-01-16
==========

Achievements
------------

- wrote a short guide for how to install bochs on Ubuntu with GDB support
- installed nasm
- set up an initial develop environment consisting of:
    - gcc as compiler
    - make as build system
    - nasm as assembler
    - bochs as simulator
    - bash as scripting language
- managed to boot the kernel in bochs using the following two guides:
    - http://www.jamesmolloy.co.uk/tutorial_html/1.-Environment%20setup.html
    - wiki.osdev.org/Bare_bones
- started working on a framebuffer device driver, wrote the two functions
  fb_write and fb_clear
- started investigating on how to improve the booting of the kernel

Issues
------

- the current boot sequence must automize how grub executes the kernel
- the media used for the OS should probably be an ISO image instead of a 
  floppy image if we ever want to boot the OS on a real computer


2012-01-17
=========

Achievements
------------

- read up on x86 assembly, nasm, cdecl, and figured out how to call assembler
  code from c
- wrote short functions to read and write to io ports
- wrote more on the framebuffer device driver, perhaps completed it
    - can move the cursor
    - can print chars at cursor position (putb) and print a string (puts)
    - newlines are handled, and scrolling
- wrote a new floppy-creator script (src/create_image.sh) that makes it easier
  to start the os - just type make run (no need to tell grub start block or
  size any more
- started to read up on segmentation and gdt/idt stuff

Issues
------

- the current floppy-build script requires sudo (to mount and use loopback
  devices), and it might cause nautilus (on ubuntu) to open the mounted
  directory, and complain when it is unmounted (can be turned off though)

2012-01-18
==========

Achievements
------------

- the gdt loads correctly. yay!
- wrote code to remap pic interrupts into the idt
- started to write code to initialize the idt

Issues
------

- the documentation on the x86 architecture is sparse at best - pic i/o port
  address, especially. We take the values from a tutorial

2012-01-19
==========

Achievements
------------

- interrupts is finally working correctly. This took a while to implement, 
  mostly due to having to deal with the PIC and the keyboard.
- added code for printing integers
- improved the build scripts slightly
- joined the =osdev irc channel on freenode where we got help with some 
  keyboard hardware details. we also got help on the os-dev forum

Issues
------

- We are still having problems with the lack of documentation for various 
  hardware, these are the issues that takes the longest time to solve, since we 
  often have to ask for help on irc or os-dev

2012-01-20
==========

Achievements
------------

- the keyboard driver is more or less finished (all normal keys are covered, 
  but support for numpad, F1 - F12, Insert, Home etc are missing). We will add 
  support for them later on if needed.
- the PIT (Programmable Interrupt Timer) driver seems to be finished 
  (hard to test), at least the timer 
  fires at different intervals depending on the frequency you set via the 
  driver.
- started reading up on paging. Also implemented full support for GRUB 
  multiboot info. We can now find reserved and available memory areas, which 
  is crucial to set uo the page table.

Issues
------

- Testing things such as timers will be hard. They seems to work correctly, but 
  it is hard to know if they fire at the exact correct interval.
- We will have to implement a function that print integers in hexadecimal 
  notation to ease debugging.

Notes
-----

- Since the basic infrastucture is now working (console, keyboard, timer), 
  the road ahead is not as clear any longer. Now we will probably have to read 
  a bit more theory and also start to discuss how we want to design the kernel,
  since at the moment we do not need any more low-level plumbing.

2012-01-21
==========

Achievements
------------

- wrote a function that prints integers in hexadecimal notation

2012-01-22
==========

Achievements
------------

- wrote a simple printf() based on the one in K&R
- added configuration file for doxygen
- wrote some doxygen documentation

Issues
------

- We need to document some more :D

2012-01-23
==========

Achievements
------------

- wrote and enabled identity (1:1) paging for the first 4MB of memory (this
  is where the kernel resides)
- rewrote the paging so that the kernel is an "upper-half" ("higher-half")
  kernel, which means that memory from 0xC0000000..0XFFFFFFFF is mapped to
  the kernel (the kernel is still at the physical memory at 1MB)

Issues
------

- We need to write more documentation on the paging and upper-half kernel
  stuff.

2012-01-24
==========

Achievements
------------

- wrote a script to generate a bootable ISO file with GRUB 2
- rewrote the script to generate a bootable ISO file with GRUB 1 and possibly
  arbitrary binary modules that the kernel load into memory
- we now only depend on genisoimage to create the bootable media
- managed to jump to a loaded module and perform an INT 0x86 instruction to 
  trigger a sys call interrupt
- make can now automatically build all modules and copy them to the iso

Issues
------

- We were not able to jump back to the kernel code after jumping loaded binary 
  code. This will require more investigation.
- The document about paging and kernel is still not written, Erik will write it 
  tomorrow morning.

2012-01-25
==========

Achievements
------------

- wrote text about paging, the kernel, and putting the kernel in the upper half
  of memory
- configured pandoc/markdown2pdf to generate html and pdf from the markdown
  files
- changed the interrupt number for syscalls to 0xAE
- we can call modules loaded by GRUB with call, and they can return to the
  kernel with ret
- determined to change the kernel stack to the end of the kernels 4MB page.
  Started to write code to relocate the modules loaded by GRUB to make it
  safe to put the stack there. The code written today creates an identity
  mapping of all memory (except for the kernels upper-half place), and uses a
  small temporary stack in bss. No module moving as of yet.

2012-01-26
==========

Achievements
------------

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

Issues
------

None, as we learn more and more about how everything works together, the issues
are more architechtural and less "low-level", which is a nice change!
