# Frame-Feed-Emulator-V4l2-Driver

**V4L2 driver with Frame Feed Emulator**
Frme Feed Emulator generates video frames of formats RGB3 and YUV2. The user can stream this generated frames through v4l2 driver using v4l2 api calls.

Tested Platform : Ubuntu 18.04.5 LTS (4.15.0-141-generic)

Dependencies : v4l2-utils

1. Build the project using make.

		$ make


2. Insert module

		$ sudo insmod frame_feed_emulator.ko
		
		$ sudo insmod driver_v4l2.ko


3. dmesg will give the node name

		$ dmesg


4. play test video using any tools

	a) FFPLAY
	
		$ ffplay /dev/video1
		
		$ ffplay -framerate 30 /dev/video1
		
		$ ffplay -video_size 1280x720 /dev/video1
	
	b) MPLAYER
		
		$ mplayer tv:// -tv driver=v4l2:device=/dev/video1:width=1280:height=720:fps=30:outfmt=yuy2
		
		$ mplayer tv:// -tv driver=v4l2:device=/dev/video1:width=1280:height=720:fps=30:outfmt=mjpg
		
	c) GStreamer Pipeline

     	$ gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-raw,interlace-mode=interleaved,height=720,width=1280 ! videoconvert ! videoscale ! autovideosink


5. Remove module
		
		$ sudo rmmod driver_v4l2

		$ sudo rmmod frame_feed_emulator


