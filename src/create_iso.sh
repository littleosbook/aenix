#!/bin/bash
# Creates a bootable ISO for aenix.
# Dependencies: xorriso and grub-mkrescue
# NOTE: grub-mkrescue is part of GRUB 2, so if your OS doesn't use GRUB 2 as 
#       the bootloader, you will not have grub-mkrescue and it will probably 
#       be a little bit of trouble installing it without removing your old
#       bootloader.

set -e # fail as soon as one command fails

# check if the program xorriso is in the path
XORRISO_PATH=`which xorriso`
if [ $XORRISO_PATH == "" ]; then
    echo "ERROR: The program xorriso must be installed\n"
    exit 1
fi

# check if the program grub-mkrescue is in the path
GRUB_MKRESCUE_PATH=`which grub-mkrescue`
if [ $GRUB_MKRESCUE_PATH == "" ]; then
    echo "ERROR: The program grub-mkrescue must be installed\n"
    exit 1
fi


# copy the kernel to the correct location
KERNEL=kernel.elf
mkdir -p iso/boot/grub
cp $KERNEL iso/boot/$KERNEL

# create the grub.cfg file
GRUB_CFG="set default=0
set timeout=0

menuentry \"aenix\" {
    multiboot /boot/$KERNEL
    boot
}"
echo "$GRUB_CFG" > iso/boot/grub/grub.cfg

# create the iso with the help of GRUB
grub-mkrescue -o aenix.iso iso

# clean up
rm -r iso
