#!/bin/bash


export ARCH=arm
export CROSS_COMPILE=/home/taba1uga/Github/learning_stm32mp135f_linux/third_party/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
export CONFIG_ARCH_STM32=y
export CONFIG_ARCH_MULTI_V7=y
export CONFIG_MACH_STM32MP13=y
export CONFIG_MACH_STM32F429=n
export CONFIG_MACH_STM32F469=n
export CONFIG_MACH_STM32F746=n
export CONFIG_MACH_STM32F769=n
export CONFIG_MACH_STM32H743=n
export CONFIG_MACH_STM32MP157=n
## set default config
# make stm32_defconfig
# make multi_v7_defconfig


## build linux
# make -j 8 all st/stm32mp135f-dk.dtb
