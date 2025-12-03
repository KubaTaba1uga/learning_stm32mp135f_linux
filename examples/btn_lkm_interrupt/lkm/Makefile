ifneq ($(KERNELRELEASE),)
  # kbuild part of makefile
  obj-m := btn_lkm_irq.o
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
