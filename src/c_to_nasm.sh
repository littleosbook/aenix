#!/bin/bash

# Translates a C header to a NASM header
# The C header can only make use of:
#   - #define
#   - #ifndef
#   - #endif

set -e

C_HEADER=$1
NASM_HEADER="${C_HEADER%%.h}.inc"

sed 's/\/\*/;/' $C_HEADER | # remove comments on single lines
sed 's/\*\///'             |
sed 's/^#/%/'              > $NASM_HEADER
#sed "s/$C_DEFINE/%/" $NASM_HEADER > $NASM_HEADER
