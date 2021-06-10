# Frame-Feed-Emulator-V4l2-Driver

**V4L2 driver with Frame Feed Emulator**

Frame Feed Emulator emulate video frames of video format YUYV422 with different sizes 480x270, 640x360, 1280x720, 1920x1080, and 3840x2160. The user can stream this test frames through v4l2 driver using v4l2 api calls. FFApp can be used to initialize FFE and stream video frames at user side.

Tested Platform : Ubuntu 20.04.2 LTS (5.8.0-55-generic)

Dependencies : v4l2-utils

1. Extract raw video files in sample directory

		cat sample/video_3840x2160.tar.xz.parta* > sample/video_3840x2160.tar.xz && tar -xf sample/video_3840x2160.tar.xz -C sample

2. Build the project using make command

		make

3. Compile ff_app

		gcc user/ff_app.c -o FFApp

4. Insert kernel modules

		sudo insmod FFE/frame_feed_emulator.ko && sudo insmod V4L2D/driver_v4l2.ko

5. dmesg will give the video node name

		dmesg

6. Run FFApp with video device path as a command line argument

		sudo ./FFApp /dev/video<node_number>

7. Remove modules
		
		sudo rmmod driver_v4l2 && sudo rmmod frame_feed_emulator
