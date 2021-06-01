# Frame-Feed-Emulator-V4l2-Driver

**V4L2 driver with Frame Feed Emulator**

Frame Feed Emulator emulate video frames of video format YUYV422 with different sizes 480x270, 640x360, 1280x720, 1920x1080, and 3840x2160. The user can stream this test frames through v4l2 driver using v4l2 api calls.

Tested Platform : Ubuntu 18.04.5 LTS (5.4.0-73-generic)

Dependencies : v4l2-utils

1. Extract raw video files in sample directory using make command

		$ make sample

2. Build the project using make command

		$ make

3. Insert kernel modules

		$ sudo insmod FFE/frame_feed_emulator.ko
		$ sudo insmod V4L2D/driver_v4l2.ko

4. dmesg will give the node name

		$ dmesg

5. Initialize the FFE with frame rate and frame count required

		$ sudo ./user/ff_initialize <frame_rate> <frame_count>

6. For listing supported video formats

		$ v4l2-ctl -d<node_number> --list-formats-ext

7. play test video using any tools

	a) FFPLAY
	
		$ ffplay -pixel_format yuyv422 -video_size <WIDTHxHEIGHT> /dev/video<node_number>
		
	c) GStreamer Pipeline

     	$ gst-launch-1.0 v4l2src device=/dev/video<node_number> ! video/x-raw,interlace-mode=interleaved,width=<WIDTH>,height=<HEIGHT> ! videoconvert ! videoscale ! autovideosink

8. Remove modules
		
		$ sudo rmmod driver_v4l2

		$ sudo rmmod frame_feed_emulator
