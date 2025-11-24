#!/bin/bash

set -xeu

export ARCH=arm
export CROSS_COMPILE=/home/taba1uga/Github/learning_stm32mp135f_linux/third_party/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-

make -j 8 zImage st/stm32mp135f-dk.dtb

cp arch/arm/boot/zImage /srv/tftp
cp arch/arm/boot/dts/st/stm32mp145f-dk.dtb /srv/tftp
sudo chmod 777 /srv/tftp/*
setenv bootargs console=ttySTM0,115200 debug ignore_loglevel initcall_debug
'console=ttySTM0,115200 earlycon loglevel=8 initcall_debug'

bind /soc/bus@5c007000/usb@49000000 usb_ether; tftp 0xC0200000 zImage; tftp 0xC2400000 stm32mp135f-dk.dtb; setenv bootargs console=ttySTM0,115200 debug ignore_loglevel earlycon; bootz 0xC0200000 - 0xC2400000
