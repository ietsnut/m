#!/bin/bash

./.tool/bin/cosmocc -g0 -I./.tool/include -o m.temp m.c

zip -q -r m.zip resource/
SIZE=$(stat -c%s m.temp)
cat m.temp m.zip > m.exe
printf $(printf '\\x%02x' $(($SIZE & 0xFF)) $((($SIZE >> 8)  & 0xFF)) $((($SIZE >> 16) & 0xFF)) $((($SIZE >> 24) & 0xFF))) >> m.exe

rm m.temp m.zip
chmod +x m.exe

read -n1 -s -r -p "Press to execute."
echo
./m.exe
