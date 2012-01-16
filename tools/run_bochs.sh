#!/bin/bash

# Run bochs with the correct bochsrc file

LOG=bochslog.txt

cd bochs

if [-e $LOG]; then
    rm $LOG
fi
    
bochs -f bochsrc.txt -q
