/* SPDX-License-Identifier: GPL */

#ifndef DRIVER_V4L2_H
#define DRIVER_V4L2_H

struct ffe_frame {
	int frame_no;
	u8 *data;
	struct ffe_frame *next;
};

struct ffe_data {
	int framerate;
	int framecount;
	int width;
	int height;
};

extern struct ffe_frame *V_BUF;
extern struct ffe_data F_DATA;
extern bool I_FLAG;
#endif
