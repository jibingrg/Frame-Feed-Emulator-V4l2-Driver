// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define COLOR_WHITE			{ 0xFF, 0xFF, 0xFF}
#define COLOR_YELLOW			{ 0xFF, 0xFF, 0x00}
#define COLOR_CYAN			{ 0x00, 0xFF, 0xFF}
#define COLOR_GREEN			{ 0x00, 0xFF, 0x00}
#define COLOR_MAGENTA			{ 0xFF, 0x00, 0xFF}
#define COLOR_RED			{ 0xFF, 0x00, 0x00}
#define COLOR_BLUE			{ 0x00, 0x00, 0xFF}
#define COLOR_BLACK			{ 0x00, 0x00, 0x00}

static const u8 rgb[8][3] = {
	COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK
};

u8 yuv[8][3];
u8 *row;
int pos;

static void ffe_initialize(unsigned int width, unsigned int pixelsize);
EXPORT_SYMBOL(ffe_initialize);
static void ffe_generate(unsigned int width, unsigned int height, unsigned int pixelsize, void *vbuf);
EXPORT_SYMBOL(ffe_generate);

static void ffe_initialize(unsigned int width, unsigned int pixelsize)
{
	int start, end;
	int i, j;
	bool flag;
	u8 *p;

	row = kmalloc(width * pixelsize << 1, GFP_KERNEL);
	for (i = 0; i < 16; i++) {
		start = (i * width) >> 3;
		end = ((i+1) * width) >> 3;
		p = row + start * pixelsize;
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

	size = width * pixelsize;
	p = row + (pos % width) * pixelsize;

	if (!vbuf) {
		pr_err("%s: buffer error..\n", __func__);
		return;
	}

	for (i = 0; i < height; i++)
		memcpy(vbuf + i * size, p, size);

	pos += 2;
}

static int __init ffe_init(void)
{
	int i;

	pr_info("%s\n", __func__);
	for (i = 0; i < 8; i++) {
		yuv[i][0] = ((66 * rgb[i][0] + 129 * rgb[i][1] + 25 * rgb[i][2] + 128) >> 8) + 16;
		yuv[i][1] = ((-38 * rgb[i][0] - 74 * rgb[i][1] + 112 * rgb[i][2] + 128) >> 8) + 128;
		yuv[i][2] = ((112 * rgb[i][0] - 94 * rgb[i][1] - 18 * rgb[i][2] + 128) >> 8) + 128;
	}
	return 0;
}


static void __exit ffe_exit(void)
{
	pr_info("%s\n", __func__);
	kfree(row);
}


module_init(ffe_init);
module_exit(ffe_exit);
