# Kernel part

obj-m += reflect.o

KERNELVER ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNELVER)/build

all: modules testtool

modules:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	make -C test clean

testtool::
	make -C test
test:: testtool
	make -C test test
