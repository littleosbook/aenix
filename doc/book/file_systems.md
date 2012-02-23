# File systems

## A simple file system

TODO: in-memory, use GRUB module. Read-only. Files and directories as blocks
with headers

## inodes and writable file systems

TODO: link to info on inodes. still in-memory 

## A virtual file system

TODO: why VFS. link to kleiman.

## Devfs

What abstraction should we use for reading and writing to devices such as the
screen and the keyboard?

TODO: /dev, and files therin. Link to devfs page.

## Persistent media

Writing the driver code to make it possible to have a file system on persistent
media such as hard drives and CD-roms could be an interesting part to implement
in the kernel.

## Further reading

- The original paper on the concept of vnodes and a virtual file system is
  quite interesting:
  <http://www.arl.wustl.edu/~fredk/Courses/cs523/fall01/Papers/kleiman86vnodes.pdf>
- The ideas behind the Plan 9 operating systems is worth taking a look at:
  <http://plan9.bell-labs.com/plan9/index.html>
- devfs
- More
