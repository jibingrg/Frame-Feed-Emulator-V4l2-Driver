# Frame-Feed-Emulator-V4l2-Driver

**V4L2 driver with Frame Feed Emulator**

Frame Feed Emulator emulate video frames of different video formats YUYV-480x270, YUYV-640x360, YUYV-1280x720, RGB-480x270, and RGB-640x360. The user can stream this test frames through v4l2 driver using v4l2 api calls.

Tested Platform : Ubuntu 18.04.5 LTS (4.15.0-141-generic)

Dependencies : v4l2-utils

1. Extract raw video files in sample directory

		$ cd sample
		$ cat video_3840x2160.tar.xz.parta* > video_3840x2160.tar.xz && tar -xf video_3840x2160.tar.xz
		$ cd ..

2. Build the project using make

		$ make

3. Insert FFE module to initiate Frame Feed Emulator

		$ sudo insmod FFE/frame_feed_emulator.ko

4. Insert driver module

		$ sudo insmod V4L2D/driver_v4l2.ko

5. dmesg will give the node name

		$ dmesg

6. For listing supported video formats

		$ v4l2-ctl -d1 --list-formats-ext

7. play test video using any tools

	a) FFPLAY
	
		$ ffplay -video_size 640x360 -framerate 30 /dev/video1
		
	c) GStreamer Pipeline

     	$ gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-raw,interlace-mode=interleaved,height=360,width=640 ! videoconvert ! videoscale ! autovideosink

8. Remove modules
		
		$ sudo rmmod driver_v4l2

		$ sudo rmmod frame_feed_emulator
