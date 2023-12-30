KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR  ?= /lib/modules/$(KERNELRELEASE)/build/
EXTRA_CFLAGS += -I`pwd`
VERSION = 1.0

obj-m += korbo.o

all:
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	make -C $(KERNEL_DIR) M=`pwd` clean

load: unload
	$(shell sudo insmod $(OUTPUT)/$(TARGET).ko)

unload:
	$(shell sudo rmmod $(OUTPUT)/$(TARGET).ko)

dkms:
	sudo mkdir /usr/src/korbo-$(VERSION)/
	sudo cp -rf `pwd`/* /usr/src/korbo-$(VERSION)/
	sudo dkms add korbo/1.0
