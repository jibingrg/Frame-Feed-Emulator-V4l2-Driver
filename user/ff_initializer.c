// SPDX-License-Identifier: GPL

#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAX_WIDTH		3840
#define MAX_HEIGHT		2160
#define FPS_DEFAULT		25
#define FCOUNT_DEFAULT		120

struct ffe_data {
	int framerate;
	int framecount;
};

int str_to_int(char *p)
{
	int i, ret = 0;

	for (i = 0; p[i]; i++) {
		if ((p[i] > '9') || (p[i] < '0')) {
			printf("invalid argument..\n");
			return 0;
		}
		ret = ret * 10 + p[i] - '0';
	}
	return ret;
}

int main(int argc, char *argv[])
{
	struct ffe_frame *head, *p, *q;
	struct ffe_data data;
	int fd1, fd2, i, j;
	unsigned int bytesperline = 2 * MAX_WIDTH;
	unsigned char *buf = malloc(bytesperline);

	if (!buf) {
		printf("Memory allocation error..\n");
		return -1;
	}

	if (argc == 2) {
		data.framerate = str_to_int(argv[1]);
		data.framecount = FCOUNT_DEFAULT;
	} else if (argc == 3) {
		data.framerate = str_to_int(argv[1]);
		data.framecount = str_to_int(argv[2]);
	} else {
		data.framerate = FPS_DEFAULT;
		data.framecount = FCOUNT_DEFAULT;
		printf("inavid number of arguments..\n");
	}

	if (!data.framerate)
		data.framerate = FPS_DEFAULT;

	if ((!data.framecount) || (data.framecount > FCOUNT_DEFAULT))
		data.framecount = FCOUNT_DEFAULT;

	printf("frame rate = %d, frame count = %d\n", data.framerate, data.framecount);

	fd1 = open("sample/video_3840x2160.yuv", O_RDONLY);
	if (fd1 < 0) {
		printf("Sample file open error..\n");
		return -1;
	}

	fd2 = open("/dev/FFE", O_RDWR);
	if (fd2 < 0) {
		printf("FFE open error..\n");
		return -1;
	}

	printf("Initializing FFE..\n");
	ioctl(fd2, 0, &data);

	for (i = 0; i < data.framecount; i++) {
		for (j = 0; j < MAX_HEIGHT; j++) {
			read(fd1, buf, bytesperline);
			write(fd2, buf, bytesperline);
		}
	}

	printf("%d frames are inserted\n", i);
	ioctl(fd2, 1, 0);
	close(fd1);
	close(fd2);
	free(buf);

	return 0;
}
