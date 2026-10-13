#!/bin/sh
# Compila el cargador y lo deja en data/booter.bin (se embebe en el juego).
# Uso (desde la raíz del repo, en el MSYS2 de devkitPro): sh tools/booter/build.sh
set -e
CC=$DEVKITPPC/bin/powerpc-eabi-gcc
OBJCOPY=$DEVKITPPC/bin/powerpc-eabi-objcopy
cd "$(dirname "$0")"
$CC -O2 -mcpu=750 -meabi -mhard-float -nostdlib -ffreestanding -fno-builtin \
    -Wl,-Ttext=0x93000000 -Wl,-e,_start -Wl,--section-start=.text.start=0x93000000 \
    -o booter.elf crt0.S booter.c
$OBJCOPY -O binary booter.elf ../../data/booter.bin
rm booter.elf
ls -l ../../data/booter.bin
