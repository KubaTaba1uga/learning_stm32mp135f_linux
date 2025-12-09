ifneq ($(KERNELRELEASE),)
  # kbuild part of makefile
  obj-m := my_bus.o
  obj-m += my_device.o
  obj-m += my_driver.o

else
  # normal makefile
  KDIR ?= /lib/modules/`uname -r`/build
  PWD  := $(shell pwd)

  ccflags-y += -DDEBUG

  all:
	  make -C $(KDIR) M=$(PWD) modules 
  install:
	  make -C $(KDIR) M=$(PWD) modules_install
  clean:
	  make -C $(KDIR) M=$(PWD) clean
endif
