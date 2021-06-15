// SPDX-License-Identifier: GPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAX_WIDTH		3840
#define MAX_HEIGHT		2160
#define MAX_FPS			125
#define FPS_DEFAULT		25
#define FCOUNT_MIN		80
#define FCOUNT_DEFAULT		120
#define FCOUNT_MAX		240

#define COLOR_WHITE		{ 0xFF, 0xFF, 0xFF}
#define COLOR_YELLOW		{ 0xFF, 0xFF, 0x00}
#define COLOR_CYAN		{ 0x00, 0xFF, 0xFF}
#define COLOR_GREEN		{ 0x00, 0xFF, 0x00}
#define COLOR_MAGENTA		{ 0xFF, 0x00, 0xFF}
#define COLOR_RED		{ 0xFF, 0x00, 0x00}
#define COLOR_BLUE		{ 0x00, 0x00, 0xFF}
#define COLOR_BLACK		{ 0x00, 0x00, 0x00}

static const unsigned char bar[8][3] = {
	COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK
};

struct ffe_data {
	int framerate;
	int framecount;
	int width;
	int height;
} data;

char gst[200];

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
	int fd1, fd2;
	int i, j, k;
	int fract;
	unsigned int size1, size2;

	if (argc == 2) {
		char gst1[] = "gst-launch-1.0 v4l2src device=";
		char gst2[] = " ! videoconvert ! autovideosink";

		strcat(gst, gst1);
		strcat(gst, argv[1]);
		strcat(gst, gst2);

		printf("\n");
		for (i = 0; gst[i]; i++)
			printf("%c", gst[i]);
		printf("\n\n");

		system(gst);
		return 0;
	}

	if (argc < 5) {
		printf("Expecting command line arguments..\n");
		printf("sudo ./FFApp <I/G> <frame_rate> <frame_count> <pixel_height>\n");
		return -1;
	}

	fd1 = open("/dev/FFE", O_RDWR);
	if (fd1 < 0) {
		printf("FFE open error..\n");
		return -1;
	}

	data.framerate = str_to_int(argv[2]);
	if ((!data.framerate) || (data.framerate > MAX_FPS))
		data.framerate = MAX_FPS;

	data.framecount = str_to_int(argv[3]);
	if (argv[1][0] == 'G') {
		if (data.framecount <= FCOUNT_MIN)
			data.framecount = FCOUNT_MIN;
		else if (data.framecount <= FCOUNT_DEFAULT)
			data.framecount = FCOUNT_DEFAULT;
		else
			data.framecount = FCOUNT_MAX;
	} else if ((!data.framerate) || (data.framecount > FCOUNT_DEFAULT))
		data.framecount = FCOUNT_DEFAULT;

	data.height = str_to_int(argv[4]);
	if (data.height <= 270)
		data.height = 270;
	else if (data.height <= 360)
		data.height = 360;
	else if (data.height <= 720)
		data.height = 720;
	else if (data.height <= 1080)
		data.height = 1080;
	else
		data.height = 2160;

	data.width = data.height * 16 / 9;
	ioctl(fd1, 0, &data);
	size1 = MAX_WIDTH << 1;
	size2 = data.width << 1;
	fract = MAX_WIDTH / data.width;

	printf("%s: frame rate = %d, frame count = %d\n", __func__, data.framerate, data.framecount);
	printf("%s: frame width = %d, frame height = %d\n", __func__, data.width, data.height);

	if (argv[1][0] == 'I') {
		unsigned char *buf1, *buf2;

		fd2 = open("sample/video_3840x2160.yuv", O_RDONLY);
		if (fd2 < 0) {
			printf("Sample file open error..\n");
			close(fd1);
			return -1;
		}

		buf1 = malloc(size1);
		buf2 = malloc(size2);

		for (i = 0; i < data.framecount; i++) {
			for (j = 0; j < MAX_HEIGHT; j++) {
				unsigned char *p = buf2;

				read(fd2, buf1, size1);
				if (j % fract)
					continue;
				for (k = 0; k < size1; k += (fract << 2)) {
					memcpy(p, buf1 + k, 4);
					p += 4;
				}
				write(fd1, buf2, size2);
			}
		}

		free(buf1);
		free(buf2);
		close(fd2);
	} else if (argv[1][0] == 'G') {
		int flag, start, end;
		unsigned char yuv[8][3];
		unsigned char colorbar[data.width << 2];
		unsigned char *p;

		for (i = 0; i < 8; i++) {
			yuv[i][0] = (((16829 * bar[i][0] + 33039 * bar[i][1] + 6416 * bar[i][2]  + 32768) >> 16) + 16);
			yuv[i][1] = (((-9714 * bar[i][0] - 19070 * bar[i][1] + 28784 * bar[i][2] + 32768) >> 16) + 128);
			yuv[i][2] = (((28784 * bar[i][0] - 24103 * bar[i][1] - 4681 * bar[i][2]  + 32768) >> 16) + 128);
		}

		for (i = 0; i < 16; i++) {
			start = i * data.width >> 3;
			end = (i + 1) * data.width >> 3;
			flag = 1;
			p = &colorbar[start << 1];

			for (j = start; j < end; j++) {
				*p = yuv[i % 8][0];
				*(p+1) = flag ? yuv[i % 8][1] : yuv[i % 8][2];
				flag = flag ? 0 : 1;
				p += 2;
			}
		}

		for (i = 0; i < data.framecount; i++) {
			p = colorbar + (((i * data.width / data.framecount) % data.width) << 1);
			for (j = 0; j < data.height; j++)
				write(fd1, p, size2);
		}
	} else {
		printf("Expeting second argument as\n\t'I' to insert frames from sample file, or\n\t'G' to generate frames");
		close(fd1);
		return -1;
	}

	printf("\nTotal %d frames are inserted\n", i);
	ioctl(fd1, 1, 0);
	close(fd1);

	printf("\nRun FFApp with video node as commandline argument to stream video\n");
	printf("./FFApp <video device node>\n");
	return 0;
}
