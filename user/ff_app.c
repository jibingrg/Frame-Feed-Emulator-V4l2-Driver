/* SPDX-License-Identifier: GPL */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAX_WIDTH		3840
#define MAX_HEIGHT		2160
#define MAX_FPS			200
#define FPS_DEFAULT		25
#define FCOUNT_DEFAULT		120

struct ffe_data {
	int framerate;
	int framecount;
};

void ff_init(struct ffe_data data)
{
	int fd1, fd2, i, j;
	unsigned int bytesperline = 2 * MAX_WIDTH;
	unsigned char *buf = malloc(bytesperline);

	fd1 = open("sample/video_3840x2160.yuv", O_RDONLY);
	if (fd1 < 0) {
		printf("Sample file open error..\n");
		return;
	}

	fd2 = open("/dev/FFE", O_RDWR);
	if (fd2 < 0) {
		printf("FFE open error..\n");
		return;
	}

	ioctl(fd2, 0, &data);

	for (i = 0; i < data.framecount; i++) {
		for (j = 0; j < MAX_HEIGHT; j++) {
			read(fd1, buf, bytesperline);
			write(fd2, buf, bytesperline);
		}
	}

	printf("\nTotal %d frames are inserted\n", i);
	ioctl(fd2, 1, 0);
	close(fd1);
	close(fd2);
	free(buf);
}

int main(int argc, char *argv[])
{
	struct ffe_data data;
	int cmd, vsize, i;
	char gst[200], device[20];
	char gst1[] ="gst-launch-1.0 v4l2src device=";
	char gst2[] =" ! video/x-raw,interlace-mode=interleaved,";
	char gst3[] =" ! videoconvert ! videoscale ! autovideosink";
	char size[5][25] = {
		"width=480,height=270", "width=640,height=360", "width=1280,height=720", "width=1920,height=1080", "width=3840,height=2160"
	};

	if (argc == 1) {
		printf("Enter video device node path");
		scanf("%s", device);
	} else {
		strcpy(device, argv[1]);
	}

	while(1) {
		printf("\t0\tExit\n\t1\tInitialize FFE\n\t2\tStream video\nSelect your option: ");
		scanf("%d", &cmd);

		if (!cmd)
			break;

		if (cmd == 1) {
			printf("******************************************************\n");
			printf("Enter framerate: ");
			scanf("%d", &data.framerate);
			printf("Enter framecount: ");
			scanf("%d", &data.framecount);

			if ((!data.framerate) || (data.framerate > MAX_FPS))
				data.framerate = FPS_DEFAULT;

			if ((!data.framecount) || (data.framecount > FCOUNT_DEFAULT))
				data.framecount = FCOUNT_DEFAULT;

			printf("\nframe rate = %d\tframe count = %d\n", data.framerate, data.framecount);
			ff_init(data);
			printf("******************************************************\n");
		} else if (cmd == 2) {
			printf("******************************************************\n");
			printf("\t0\t480x270\n\t1\t640x360\n\t2\t1280x720\n\t3\t1920x1080\n\t4\t3840x2160\nSelect video size: ");
			scanf("%d", &vsize);

			if ((vsize < 0) || (vsize > 4)) {
				printf("Selected option is not valid..\n");
				continue;
			}

			for (i = 0; i < 200; i++)
				gst[i] = 0;

			strcat(gst, gst1);
			strcat(gst, device);
			strcat(gst, gst2);
			strcat(gst, size[vsize]);
			strcat(gst, gst3);
			printf("\n");
			for (i = 0; gst[i]; i++)
				printf("%c", gst[i]);
			printf("\n");
			printf("******************************************************\n");
			system(gst);
			printf("******************************************************\n");
		} else {
			printf("******************************************************\n");
			printf("Enter a valid command:\n");
			continue;
		}
	}

	return 0;
}
