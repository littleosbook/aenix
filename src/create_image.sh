#!/bin/bash

# Based on JamesM tutorial, see 
# http://www.jamesmolloy.co.uk/tutorial_html/1.-Environment%20setup.html

# This script requires sudo

BIN="bin"
GRUB="../tools/grub"
MNT="/media/aenix"
LOOPBACK="/dev/loop0"
FLOPPY="floppy.img"
KERNEL="kernel"

set -e
echo "==== Creating bootable floppy image ===="

if [ -d $BIN ]; then
    rm -r $BIN
fi

mkdir $BIN

cp $GRUB/$FLOPPY $BIN/$FLOPPY
sudo losetup $LOOPBACK $BIN/$FLOPPY
sudo mkdir $MNT
sudo mount $LOOPBACK $MNT
sudo cp $KERNEL $MNT/$KERNEL
sudo umount $MNT
sudo losetup -d $LOOPBACK
sudo rmdir $MNT

echo "==== Done ===="
