ifneq ($(KERNELRELEASE),)
  # kbuild part of makefile
  obj-m := my_device.o

else
  # normal makefile
  KDIR ?= /lib/modules/`uname -r`/build
  PWD  := $(shell pwd)

  ccflags-y += -DDEBUG

  all:
	  make -C $(KDIR) M=$(PWD) KBUILD_EXTRA_SYMBOLS=$(PWD)/../bus_lkm/Module.symvers modules 
  install:
	  make -C $(KDIR) M=$(PWD) modules_install
  clean:
	  make -C $(KDIR) M=$(PWD) clean
endif
