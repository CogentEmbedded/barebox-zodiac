#!/bin/sh

# --eval="add-symbol-file barebox 0x10000800" \

GDB=arm-none-eabi-gdb

${GDB} \
    --eval="set confirm off" \
    --eval="target remote localhost:3333" \
    --eval="restore images/barebox-freescale-imx6q-sabresd.img binary 0x10000000" \
    --eval="add-symbol-file barebox 0x10001000" \
    --eval="set \$pc=0x10001050"
    # --eval="continue"
