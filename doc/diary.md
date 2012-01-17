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
