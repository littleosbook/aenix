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
