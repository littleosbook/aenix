#!/bin/bash
# Starts bochs using the iso created by create_iso.sh

ISO=aenix.iso
BOCHS_LOG=bochslog.txt
BOCHS_CONFIG=bochsrc.txt
BOCHS_BIOS_PATH=../tools/bochs

set -e # fail as soon as one command fails

# check if an old log file exists, if so, remove it
if [ -e $BOCHS_LOG ]; then
    rm $BOCHS_LOG
fi

# check if an old config file exists, if so, remove it
if [ -e $BOCHS_CONFIG ]; then
    rm $BOCHS_CONFIG
fi

# create the config file for bochs
CONFIG="megs:           32
romimage:       file=$BOCHS_BIOS_PATH/BIOS-bochs-latest
vgaromimage:    file=$BOCHS_BIOS_PATH/VGABIOS-lgpl-latest
ata0-master:    type=cdrom, path=$ISO, status=inserted
boot:           cdrom
log:            $BOCHS_LOG
mouse:          enabled=1
clock:          sync=realtime, time0=local
cpu:            count=1, ips=1000000
com1:           enabled=1, mode=file, dev=com1.out"

echo "$CONFIG" > $BOCHS_CONFIG

# start bochs
bochs -f $BOCHS_CONFIG -q

exit 0
