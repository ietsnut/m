#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 filename"
    exit 1
fi

filename="$1"
./.tool/bin/cosmocc -g0 -I./.tool/include -o "$filename.exe" "$filename.c"
./"$filename.exe"