# Development environment and booting

Developing an operating system (OS) is no easy task, and the question " Where
do I start?" is likely to come up several times during the course of the
project.  This chapter will help you set up your development environment as
well as printing the famous `"Hello, world!"` on the console of the computer.

## Tools

### The easy way
The easiest way to get all the required tools is to use Ubuntu [@ubuntu] as
your operating system. If you don't want to run Ubuntu natively on your
computer, it works just as well running it in a virtual machine, for example in
VirtualBox [@virtualbox].

The packages needed can then be installed by running

    sudo apt-get install build-essential nasm genisoimage bochs bochs-x

### Programming language
The operating system will be developed using the C programming language
[@wikic]. The reason for using C is because developing and OS
requires a very precise control of the generated code, something which C
enables. Other languages can be used that does the same, for example C++, can
also be used, but this book will only cover C.

The code will make use of one type attribute that is specific for GCC [@gcc]

    __attribute__((packed))__

(the meaning of the attribute will be explained later).
Therefore, the example code might hard to compile using another C
compiler.

For writing assembly, we choose NASM [@nasm] as the assembler. The reason for
this is that we prefer the NASMs syntax over GNU Assembler.

### Host operating system
All the examples assumes that the code is being compiled on a UNIX like
operating system. All the code is known to compile on Ubuntu [@ubuntu] versions
11.04 and 11.10.

### Build system
GNU Make [@make] has been used when constructing the example Makefiles, but we
don't make use of any GNU Make specific instructions.
