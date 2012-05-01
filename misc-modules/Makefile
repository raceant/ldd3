#if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
DEBUG = y


ifneq ($(KERNELRELEASE),)
		obj-m := faulty.o 

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
		KERNELDIR ?= /lib/modules/$(shell uname -r)/build
		PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
