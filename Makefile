obj-m := FFE/frame_feed_emulator.o V4L2D/driver_v4l2.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	gcc user/ff_initializer.c -o user/ff_initializer
	gcc user/ff_app.c -o user/FFApp

clean :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm user/ff_initializer
	rm user/FFApp

sample : sample/video_3840x2160.tar.xz
	tar -xf sample/video_3840x2160.tar.xz -C sample

sample/video_3840x2160.tar.xz :
	cat sample/video_3840x2160.tar.xz.parta* > sample/video_3840x2160.tar.xz

sampleclean :
	rm sample/video_3840x2160.tar.xz sample/video_3840x2160.yuv
