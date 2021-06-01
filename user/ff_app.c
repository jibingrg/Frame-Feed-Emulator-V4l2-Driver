/* SPDX-License-Identifier: GPL */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[])
{
	int cmd, i;
	char gst[200], finit[50], device[20], width[10], height[10], frate[10], fcount[10];
	char space[] = " ";
	char finit1[] = "./user/ff_initializer ";
	char gst1[] ="gst-launch-1.0 v4l2src device=";
	char gst2[] =" ! video/x-raw,interlace-mode=interleaved,width=";
	char gst3[] =",height=";
	char gst4[] =" ! videoconvert ! videoscale ! autovideosink";

	if (argc == 1) {
		printf("Enter video device node path");
		scanf("%s", device);
	} else {
		strcpy(device, argv[1]);
	}

	while(1) {
		printf("Enter 0 to exit, 1 to initialize FFE, and 2 to stream video..\n");
		scanf("%d", &cmd);

		if (!cmd)
			break;

		if (cmd == 1) {
			for (i = 0; i < 50; i++)
				finit[i] = 0;
			printf("Enter framerate..\n");
			scanf("%s", frate);
			printf("Enter framecount..\n");
			scanf("%s", fcount);
			strcat(finit, finit1);
			strcat(finit, frate);
			strcat(finit, space);
			strcat(finit, fcount);
			for (i = 0; finit[i]; i++)
				printf("%c", finit[i]);
			printf("\n");
			system(finit);
		} else if (cmd == 2) {
			for (i = 0; i < 200; i++)
				gst[i] = 0;
			printf("Enter width..\n");
			scanf("%s", width);
			printf("Enter height..\n");
			scanf("%s", height);
			strcat(gst, gst1);
			strcat(gst, device);
			strcat(gst, gst2);
			strcat(gst, width);
			strcat(gst, gst3);
			strcat(gst, height);
			strcat(gst, gst4);
			for (i = 0; gst[i]; i++)
				printf("%c", gst[i]);
			printf("\n");
			system(gst);
		} else {
			printf("Enter a valid command..\n");
			continue;
		}
	}

	return 0;
}
