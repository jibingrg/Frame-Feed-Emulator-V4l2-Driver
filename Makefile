PWD := $(shell pwd)

all :
	$(MAKE) -C $(PWD)/FFE all
	$(MAKE) -C $(PWD)/V4L2D all

clean :
	$(MAKE) -C $(PWD)/FFE clean
	$(MAKE) -C $(PWD)/V4L2D clean
