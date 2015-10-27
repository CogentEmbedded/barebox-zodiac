#!/bin/sh


GDB=arm-none-eabi-gdb

${GDB} \
    --eval="set confirm off" \
    --eval="target remote localhost:3333" \
    --eval="restore barebox.bin binary 0x91e000" \
    --eval="add-symbol-file barebox 0x91e000" \
    --eval="set \$pc=0x91e050" \
    --eval="continue"
