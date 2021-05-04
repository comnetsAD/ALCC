#Filename: Makefile
CONFIG_MODULE_SIG=n
CONFIG_MODULE_SIG_ALL=n

obj-m := alcc_kernel.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm *o
