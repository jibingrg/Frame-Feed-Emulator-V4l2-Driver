/* SPDX-License-Identifier: GPL */

#ifndef DRIVER_V4L2_H
#define DRIVER_V4L2_H
struct ffe_frame {
	int frame_no;
	u8 *data;
	struct ffe_frame *next;
};
extern int FRAME_RATE;
extern struct ffe_frame *V_BUF;
extern bool I_FLAG;
#endif
