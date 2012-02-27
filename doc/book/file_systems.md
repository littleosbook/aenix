# File systems

We are not required to have file systems in our operating system, but it is
quite convenient, and often plays a central part in many operations of several
existing OS's, especially UNIX-like systems. Before we start the process of
supporting multiple processes (:D) and system calls, we might want to consider
implementing a simple file system. 

## Why a file system?

How do we specify what programs to run in our OS? Which is the first program to
run? How do programs output data? Read input?

In UNIX-like systems, with their almost-everything-is-a-file convention, these
problems are solved by the file systems. It might also be interesting here to
read about the Plan 9 project, which takes this idea one step further (See
further reading ????? below).

## A simple file system

The most simple file system possible might be what we already have - one
"file", existing only in RAM, loaded by GRUB before the kernel starts. When our
kernel and operating system grows, this is probably too limiting.

A next step could be do add structure to this GRUB-loaded module. With a
utility program - called `mkfs`, perhaps - which is run when we build the ISO
which we boot from, we can create our file system in this file.

`mkfs` can traverse a directory on our host system and add all subdirectories
and files as part of our target file system. Each object in the file system
(directory or file) can consist of a header and a body, where the body of a
file is the actual file and the body of a directory is a list of entries -
names and "addresses" of other files and directories.

Each object in this file system will become contiguous, so it will be easy to
read from our kernel. All objects will also have a fixed size (except for the
last one, which can grow), it might be difficult to add new or modify existing
files. We can make the file system read-only.

## Inodes and writable file systems

When we decide that we want a more complex - and realistic - file system, we
might want to look into the concept of inodes. See further reading ????.

## A virtual file system and devfs

What abstraction should we use for reading and writing to devices such as the
screen and the keyboard?

With a virtual file system (VFS) we create an abstraction on top of any actual
file systems we might have. The VFS mainly supplies the path system and file
hierarchy, and delegates actually dealing with files to the real file systems.
The original paper on VFS is succinct, concrete, and well worth a read. See
further reading ????.

With a VFS we can mount a special file system on `/dev`, which handles all
devices such as keyboards and the screen. Or we can take the traditional UNIX
approach, with major/minor device numbers and `mknod` to create special files
for our devices.

## Persistent media

Writing the driver code to make it possible to have a file system on persistent
media such as hard drives and CD-roms could be interesting.

## Further reading

- The ideas behind the Plan 9 operating systems is worth taking a look at:
  <http://plan9.bell-labs.com/plan9/index.html>
- Wikipedia's page on inodes: <http://en.wikipedia.org/wiki/Inode> and the
  inode pointer structure:
  <http://en.wikipedia.org/wiki/Inode_pointer_structure>.
- The original paper on the concept of vnodes and a virtual file system is
  quite interesting:
  <http://www.arl.wustl.edu/~fredk/Courses/cs523/fall01/Papers/kleiman86vnodes.pdf>
- TODO: devfs
