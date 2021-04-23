obj-m := driver_v4l2.o frame_feed_emulator.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
