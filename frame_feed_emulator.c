// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define MAX_WIDTH			1920

MODULE_LICENSE("GPL");

static int count;
static u8 raw[MAX_WIDTH * 6];
static void ffe_initialize(unsigned int width, unsigned int pixelsize);
EXPORT_SYMBOL(ffe_initialize);
static void ffe_generate(unsigned int width, unsigned int height, unsigned int pixelsize, void *vbuf);
EXPORT_SYMBOL(ffe_generate);

/* ------------------------------------ {    R,    G,    B} */

#define COLOR_WHITE			{ 0xFF, 0xFF, 0xFF}
#define COLOR_YELLOW			{ 0xFF, 0xFF, 0x00}
#define COLOR_CYAN			{ 0x00, 0xFF, 0xFF}
#define COLOR_GREEN			{ 0x00, 0xFF, 0x00}
#define COLOR_MAGENTA			{ 0xFF, 0x00, 0xFF}
#define COLOR_RED			{ 0xFF, 0x00, 0x00}
#define COLOR_BLUE			{ 0x00, 0x00, 0xFF}
#define COLOR_BLACK			{ 0x00, 0x00, 0x00}

/* ----------standard color bar---------- */
static const u8 rgb[8][3] = {
	COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK
};

u8 yuv[8][3];

static void ffe_initialize(unsigned int width, unsigned int pixelsize)
{
	int start, end;
	int i, j;
	bool flag;
	u8 *p;

	for (i = 0; i < 16; i++) {
		start = i * width / 8;
		end = (i+1) * width / 8;
		p = &raw[start * pixelsize];
		flag = true;

		for (j = start; j < end; j++) {
			if (pixelsize == 2) {
				*p = yuv[i % 8][0];
				*(p+1) = flag ? yuv[i % 8][1] : yuv[i % 8][2];
				flag = !flag;
			} else {
				memcpy(p, &rgb[i % 8][0], pixelsize);
			}
			p += pixelsize;
		}
	}
}

static void ffe_generate(unsigned int width, unsigned int height, unsigned int pixelsize, void *vbuf)
{
	int size, i;
	u8 *p;

	if (!vbuf) {
		pr_err("%s: buffer error..\n", __func__);
		return;
	}

	size = width * pixelsize;
	p = &raw[(count % width) * pixelsize];

	for (i = 0; i < height; i++)
		memcpy(vbuf + i * size, p, size);

	count += 2;
}

static int __init ffe_init(void)
{
	int i;

	pr_info("%s\n", __func__);
	for (i = 0; i < 8; i++) {
		yuv[i][0] = ((16829 * rgb[i][0] + 33039 * rgb[i][1] + 6416 * rgb[i][2] + 32768) >> 16) + 16;
		yuv[i][1] = ((-9714 * rgb[i][0] - 19070 * rgb[i][1] + 28784 * rgb[i][2] + 32768) >> 16) + 128;
		yuv[i][2] = ((28784 * rgb[i][0] - 24103 * rgb[i][1] - 4681 * rgb[i][2]  + 32768) >> 16) + 128;
	}
	return 0;
}


static void __exit ffe_exit(void)
{
	pr_info("%s\n", __func__);
}


module_init(ffe_init);
module_exit(ffe_exit);
