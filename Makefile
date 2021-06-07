obj-m += FFE/frame_feed_emulator.o V4L2D/driver_v4l2.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
