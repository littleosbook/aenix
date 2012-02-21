# Booting up

Developing an operating system (OS) is no easy task, and the question " Where
do I start?" is likely to come up several times during the course of the
project.  This chapter will help you set up your development environment as
well as printing the famous `"Hello, world!"` on the console of the computer.

## Requirements
In this book, we will assume that you are familiar with the C programming
language and that you have some programming experience. It will be also be
helpful if you have basic understanding of assembly code.

## Tools

### The easy way
The easiest way to get all the required tools is to use Ubuntu [@ubuntu] as
your operating system. If you don't want to run Ubuntu natively on your
computer, it works just as well running it in a virtual machine, for example in
VirtualBox [@virtualbox].

The packages needed can then be installed by running

    sudo apt-get install build-essential nasm genisoimage bochs bochs-x

### Programming languages
The operating system will be developed using the C programming language
[@wiki:c]. The reason for using C is because developing and OS requires a very
precise control of the generated code and direct access to the memory,
something which C enables. Other languages can be used that does the same, for
example C++, can also be used, but this book will only cover C.

The code will make use of one type attribute that is specific for GCC [@gcc]

    __attribute__((packed))__

(the meaning of the attribute will be explained later).
Therefore, the example code might hard to compile using another C
compiler.

For writing assembly, we choose NASM [@nasm] as the assembler. The reason for
this is that we prefer the NASMs syntax over GNU Assembler.

Shell [@wiki:shell] will be used as the scripting language throughout the book.

### Host operating system
All the examples assumes that the code is being compiled on a UNIX like
operating system. All the code is known to compile on Ubuntu [@ubuntu] versions
11.04 and 11.10.

### Build system
GNU Make [@make] has been used when constructing the example Makefiles, but we
don't make use of any GNU Make specific instructions.

### Virtual machine
When developing an OS, it's very convenient to be able to run your code in a
virtual machine instead of a physical one. Bochs [@bochs] is emulator for the
x86 (IA-32) platform which is well suited for OS development due to its
debugging features. Other popular choices are QEMU [@qemu] and VirtualBox
[@virtualbox], but we will use Bochs in this book.

## Booting
The first goal when starting to develop an operating system is to be able to
boot it. Booting an operating system consists of transferring control to a
chain of small programs, each one more "powerful" than the previous one, where
the operating system is the last "program". See figure 1-1 for an overview of
the boot process.

![Figure 1-1: An overview of the boot process, each box is a program.](images/boot_chain.png)

### Powering on the computer
When the PC is turned on, the computer will start a small program called that
adheres to the Basic Input Output System (BIOS) [@wiki:bios] standard.
The program is usually
stored in a read only memory chip on the motherboard of the PC. The
original role of
the BIOS program was export some library functions for printing to the screen,
reading keyboard input etc. However, todays operating system does not use the
BIOS functions, instead they interact with the hardware using drivers that
interacts directly with the hardware, bypassing the BIOS [@wiki:bios].

BIOS is a legacy technology, it operates in 16-bit mode (all x86 CPUs are
backwards compatible with 16-bit mode). However, they still have one very
important task to perform, and that is transferring control to the next program
in the chain.

### The bootloader
The BIOS program will transfer control of the PC to a program called
_bootloader_. The bootloaders task is to load the to transfer control to us, the
operating system developers, and our code. However, due to some legacy
restrictions, the bootloader is often split into two parts, so the first part
of the bootloader will transfer control to the second part which will finally
give the control of the PC to the operating system.

Writing a bootloader involves writing a lot of low-level code that is
interacting with the BIOS, so in this book, we will use an existing bootloader,
the GNU GRand Unified Bootloader (GRUB) [@grub].

## Development environment
To start with, a shell script will be created as a wrapper around Bochs. The
script will clean up some log files, write the Bochs configuration file and
finally start Bochs itself.

    #!/bin/bash
    # Starts bochs using the iso created by create_iso.sh

    ISO=os.iso
    BOCHS_LOG=bochslog.txt
    BOCHS_CONFIG=bochsrc.txt
    BOCHS_BIOS_PATH=/usr/share/bochs/
    COM1_LOG=com1.out

    set -e # fail as soon as one command fails

    # check if an old log file exists, if so, remove it
    if [ -e $BOCHS_LOG ]; then
        rm $BOCHS_LOG
    fi

    # check if an old config file exists, if so, remove it
    if [ -e $BOCHS_CONFIG ]; then
        rm $BOCHS_CONFIG
    fi

    # remove the old log
    rm -f $COM1_LOG

    # create the config file for bochs
    CONFIG="megs:    32
    display_library: x
    romimage:        file=\"$BOCHS_BIOS_PATH/BIOS-bochs-latest\"
    vgaromimage:     file=\"$BOCHS_BIOS_PATH/VGABIOS-lgpl-latest\"
    ata0-master:     type=cdrom, path=$ISO, status=inserted
    boot:            cdrom
    log:             $BOCHS_LOG
    mouse:           enabled=1
    clock:           sync=realtime, time0=local
    cpu:             count=1, ips=1000000
    com1:            enabled=1, mode=file, dev=$COM1_LOG"

    echo "$CONFIG" > $BOCHS_CONFIG

    # start bochs
    bochs -f $BOCHS_CONFIG -q

    exit 0

You might have to edit the variables `$BOCHS_BIOS_PATH` to point to a different
folder. If you downloaded the `.tar.gz` release Bochs from the Bochs homepage,
the BIOSs are in the `bios` folder.

## Further reading
- Gustavo Duertes has written an in-depth article about what actually happens
  when a computers boots up,
  <http://duartes.org/gustavo/blog/post/how-computers-boot-up>
- Gustavo then continues to describe what the kernel does in the very early
  stages at <http://duartes.org/gustavo/blog/post/kernel-boot-process>
- The OSDev wiki also contains a nice article about booting a computer,
  <http://wiki.osdev.org/Boot_Sequence>
