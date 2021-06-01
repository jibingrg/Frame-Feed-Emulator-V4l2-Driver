# Frame-Feed-Emulator-V4l2-Driver

**V4L2 driver with Frame Feed Emulator**

Frame Feed Emulator emulate video frames of video format YUYV422 with different sizes 480x270, 640x360, 1280x720, 1920x1080, and 3840x2160. The user can stream this test frames through v4l2 driver using v4l2 api calls. FFApp can be used to initialize FFE and stream video frames at user side.

Tested Platform : Ubuntu 18.04.5 LTS (5.4.0-73-generic)

Dependencies : v4l2-utils

1. Extract raw video files in sample directory using make command

		$ make sample

2. Build the project using make command

		$ make

3. Insert kernel modules

		$ sudo insmod FFE/frame_feed_emulator.ko
		$ sudo insmod V4L2D/driver_v4l2.ko

4. dmesg will give the video node name

		$ dmesg

5. For listing supported video formats

		$ v4l2-ctl -d<node_number> --list-formats-ext

6. Run FFApp with video device path as a command line argument

		$ sudo ./user/FFApp /dev/video<node_number>

7. Remove modules
		
		$ sudo rmmod driver_v4l2

		$ sudo rmmod frame_feed_emulator
