#!/bin/bash

# Compile the executable with Cosmopolitan and include the path to headers
./.tool/bin/cosmocc -g0 -I./.tool/include -o m.temp m.c

#-I./.tool/include/third_party

# Create the zip archive
zip -q -r m.zip resources/

# Get the size of z.temp
SIZE=$(stat -c%s m.temp)

# Concatenate z.temp and z.zip into z.exe
cat m.temp m.zip > m.exe

# Append the size of z.exe as a 4-byte little-endian integer
printf $(printf '\\x%02x' $(($SIZE        & 0xFF)) \
                             $((($SIZE >> 8)  & 0xFF)) \
                             $((($SIZE >> 16) & 0xFF)) \
                             $((($SIZE >> 24) & 0xFF))) >> m.exe

# Make z executable
chmod +x m.exe

# Run the final executable
./m.exe
