# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the overlay filesystem.
#

# obj-$(CONFIG_OVERLAY_FS) += overlay.o

# overlay-objs := super.o namei.o util.o inode.o file.o dir.o readdir.o \
# 		copy_up.o export.o

ifneq  ($(KERNELRELEASE),)
obj-m := overlay.o

overlay-objs := super.o namei.o util.o inode.o file.o dir.o readdir.o \
                copy_up.o export.o 
else
KDIR := /lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.symvers *.cmd *.cmd.o
endif
