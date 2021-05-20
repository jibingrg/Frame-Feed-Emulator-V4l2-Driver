/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef DRIVER_V4L2_H
#define DRIVER_V4L2_H
extern void ffe_initialize(unsigned int width, unsigned int height, unsigned int pixelsize);
extern void ffe_generate(void *vbuf);
#endif
