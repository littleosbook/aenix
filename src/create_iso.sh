#!/bin/bash
# Creates a bootable ISO for aenix.
# Arguments: A list of modules to load at boot
# Dependencies: genisoimage

# check if the program grub-mkrescue is in the path
GENISOIMAGE_PATH=`which genisoimage`
if [ -z $GENISOIMAGE_PATH ]; then
    echo "ERROR: The program genisoimage must be installed"
    exit 1
fi

set -e # fail as soon as one command fails

MODULES="$@"
ISO_FOLDER="iso"
STAGE2_ELTORITO="../tools/grub/stage2_eltorito"

# create the ISO catalog structure
mkdir -p $ISO_FOLDER/boot/grub
mkdir $ISO_FOLDER/modules

# copy the stage2_eltorito into the correct place
cp $STAGE2_ELTORITO $ISO_FOLDER/boot/grub/

# copy the kernel to the correct location
KERNEL=kernel/kernel.elf
cp $KERNEL $ISO_FOLDER/boot/kernel.elf

# copy all the modules
for m in $MODULES
do
    cp $m $ISO_FOLDER/modules/
done

# create the menu.lst file

# set default=0: boot from the first entry (which will be aenix)
# set timeout=0: immediatly boot from the first entry
MENU="default=0
timeout=0
"

# add the menu entry for aenix
MENU="$MENU
title aenix
kernel /boot/kernel.elf"

# create one entry for each module
for m in $MODULES
do
    MENU="$MENU
module /modules/$m"
done


echo "$MENU" > $ISO_FOLDER/boot/grub/menu.lst

# build the ISO image
# -R:                   Use the Rock Ridge protocol (needed by GRUB)
# -b file:              The file to boot from (relative to the root folder of 
#                       the ISO)
# -no-emul-boot:        Do not perform any disk emulation
# -boot-load-size sz:   The number 512 byte sectors to load. Apparently most 
#                       BIOS likes the number 4.
# -boot-info-table:     Writes information about the ISO layout to ISO (needed 
#                       by GRUB)
# -o name:              The name of the iso
# -A name:              The label of the iso
# -input-charset cs:    The charset for the input files
# -quiet:               Disable any output from genisoimage
genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
            -A aenix -input-charset utf8 -quiet -boot-info-table \
            -o aenix.iso $ISO_FOLDER

# clean up
rm -r $ISO_FOLDER
