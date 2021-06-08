// SPDX-License-Identifier: GPL

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAX_WIDTH		3840
#define MAX_HEIGHT		2160
#define MAX_FPS			120
#define FPS_DEFAULT		25
#define FCOUNT_DEFAULT		120

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
} data;

void ff_init(void)
{
	int fd1, fd2, i, j, cmd;
	unsigned int bytesperline = 2 * MAX_WIDTH;
	unsigned char *buf = malloc(bytesperline);

	fd1 = open("/dev/FFE", O_RDWR);
	if (fd1 < 0) {
		printf("FFE open error..\n");
		return;
	}

	printf("\t0\tFeed frames from a file\n\t1\tGenerate colorbar\n Select an option: ");
	scanf("%d", &cmd);

	printf("Enter framerate: ");
	scanf("%d", &data.framerate);

	if ((!data.framerate) || (data.framerate > MAX_FPS))
		data.framerate = FPS_DEFAULT;

	if (cmd) {
		int flag;
		int start, end;
		unsigned char yuv[8][3];
		unsigned char colorbar[MAX_WIDTH * 4], *p;

		data.framecount = FCOUNT_DEFAULT;
		printf("\nframe rate = %d\tframe count = %d\n", data.framerate, data.framecount);
		ioctl(fd1, 0, &data);

		for (i = 0; i < 8; i++) {
			yuv[i][0] = (((16829 * bar[i][0] + 33039 * bar[i][1] + 6416 * bar[i][2]  + 32768) >> 16) + 16);
			yuv[i][1] = (((-9714 * bar[i][0] - 19070 * bar[i][1] + 28784 * bar[i][2] + 32768) >> 16) + 128);
			yuv[i][2] = (((28784 * bar[i][0] - 24103 * bar[i][1] - 4681 * bar[i][2]  + 32768) >> 16) + 128);
		}

		for (i = 0; i < 16; i++) {
			start = i * MAX_WIDTH / 8;
			end = (i + 1) * MAX_WIDTH / 8;
			flag = 1;
			p = &colorbar[start * 2];

			for (j = start; j < end; j++) {
				*p = yuv[i % 8][0];
				*(p+1) = flag ? yuv[i % 8][1] : yuv[i % 8][2];
				flag = flag ? 0 : 1;
				p += 2;
			}
		}

		for (i = 0; i < FCOUNT_DEFAULT; i++) {
			p = colorbar + ((i * MAX_WIDTH / FCOUNT_DEFAULT) % MAX_WIDTH) * 2;
			memcpy(buf, p, bytesperline);
			for (j = 0; j < MAX_HEIGHT; j++)
				write(fd1, buf, bytesperline);
		}
	} else {
		printf("Enter framecount: ");
		scanf("%d", &data.framecount);

		if ((!data.framecount) || (data.framecount > FCOUNT_DEFAULT))
			data.framecount = FCOUNT_DEFAULT;

		printf("\nframe rate = %d\tframe count = %d\n", data.framerate, data.framecount);
		ioctl(fd1, 0, &data);

		fd2 = open("sample/video_3840x2160.yuv", O_RDONLY);
		if (fd2 < 0) {
			printf("Sample file open error..\n");
			close(fd1);
			free(buf);
			return;
		}

		for (i = 0; i < data.framecount; i++) {
			for (j = 0; j < MAX_HEIGHT; j++) {
				read(fd2, buf, bytesperline);
				write(fd1, buf, bytesperline);
			}
		}

		close(fd2);
	}

	printf("\nTotal %d frames are inserted\n", i);
	ioctl(fd1, 1, 0);
	close(fd1);
	free(buf);
}

int main(int argc, char *argv[])
{
	int cmd, vsize, i;
	char gst[200], device[20];
	char gst1[] = "gst-launch-1.0 v4l2src device=";
	char gst2[] = " ! video/x-raw,interlace-mode=interleaved,";
	char gst3[] = " ! videoconvert ! videoscale ! autovideosink";
	char size[5][25] = {
		"width=480,height=270", "width=640,height=360", "width=1280,height=720", "width=1920,height=1080", "width=3840,height=2160"
	};

	if (argc == 1) {
		printf("Enter video device node path: ");
		scanf("%s", device);
	} else {
		strcpy(device, argv[1]);
	}

	while (1) {
		printf("\t0\tExit\n\t1\tInitialize FFE\n\t2\tStream video\nSelect your option: ");
		scanf("%d", &cmd);

		if (!cmd)
			break;

		if (cmd == 1) {
			printf("******************************************************\n");
			ff_init();
			printf("******************************************************\n");
		} else if (cmd == 2) {
			printf("******************************************************\n");
			printf("\t0\t480x270\n\t1\t640x360\n\t2\t1280x720\n\t3\t1920x1080\n\t4\t3840x2160\nSelect video size: ");
			scanf("%d", &vsize);

			if ((vsize < 0) || (vsize > 4)) {
				printf("Selected option is not valid..\n");
				break;
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
		}
	}

	return 0;
}
