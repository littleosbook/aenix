2012-01-16
##########

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
#########

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
##########

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
##########

Achievements
------------

- interrupts is finally working correctly. This took a while to implement, 
  mostly due to having to deal with the PIC and the keyboard.
- added code for printing integers
- improved the build scripts slightly
- joined the #osdev irc channel on freenode where we got help with some 
  keyboard hardware details. we also got help on the os-dev forum

Issues
------

- We are still having problems with the lack of documentation for various 
  hardware, these are the issues that takes the longest time to solve, since we 
  often have to ask for help on irc or os-dev

2012-01-20
##########

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
