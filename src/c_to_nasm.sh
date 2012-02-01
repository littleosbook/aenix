#!/bin/bash

# Translates a C header to a NASM header
# The C header can only make use of:
#   - #define
#   - #ifndef
#   - #endif

# INPUT: A list of the required NASM headers

set -e

for NASM_HEADER in $@
do
    C_HEADER="${NASM_HEADER%%.inc}.h"
    if [ $C_HEADER -nt $NASM_HEADER ]; then
        sed 's/\/\*/;/' $C_HEADER | # change start of comments
        sed 's/\*\///'            | # change end of comments
        sed 's/^#/%/'             > $NASM_HEADER
    fi
done
