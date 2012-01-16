#!/bin/bash

# Creates a floppy image that can be used with bochs.
# The floppy image consist of GRUB legacy (0.97) and the kernel

# Creates an empty padding file of exactly 750 bytes to pad the kernel 
# to the start of a floppy disc block

BIN="bin"
GRUB="../tools/grub"

echo "==== Creating bootable floppy image ===="

if [ -d $BIN ]; then
    rm -r $BIN
fi

mkdir $BIN

dd if=/dev/zero of=pad1 bs=1 count=750 2> /dev/null

cat $GRUB/stage1 $GRUB/stage2 pad1 kernel > $BIN/floppy.img

rm pad1

KERNEL_SIZE=`du --apparent-size --block-size=512 -s kernel | cut -f 1`
echo "-- starting block: 200"
echo "-- size in blocks: $KERNEL_SIZE"

echo "==== DONE ===="

# TODO
# Creates a second padding file in order to create a file that is exactly 
# 1.44 MB (the size of a floppy disk)
# dd if=/dev/zero of=pad2 bs=1 count=?
# cat grub/stage1 grub/stage2 pad1 kernel pad2 > floppy.img
