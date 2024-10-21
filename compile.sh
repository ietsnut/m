#!/bin/bash

# Compile the code
./.tool/bin/cosmocc -g0 -I./.tool/include -o m.temp m.c

# Check if the zip file already exists
if [ ! -f m.zip ]; then
    echo "Bundling resources..."
    zip -q -r m.zip resource/
fi

# Get the size of the compiled temp file
SIZE=$(stat -c%s m.temp)

# Concatenate m.temp and m.zip into m.exe
cat m.temp m.zip > m.exe

# Append the size of m.temp in little-endian format to m.exe
printf $(printf '\\x%02x' $(($SIZE & 0xFF)) $((($SIZE >> 8)  & 0xFF)) $((($SIZE >> 16) & 0xFF)) $((($SIZE >> 24) & 0xFF))) >> m.exe

# Clean up temporary files
rm m.temp

# Set executable permissions for m.exe
chmod +x m.exe

# Wait for user input to execute
read -n1 -s -r -p "Press any key to execute."
echo
./m.exe
