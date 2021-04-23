// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define MAX_WIDTH			1920

MODULE_LICENSE("GPL");

static int count;
static u8 raw[MAX_WIDTH * 6];
static void ffe_initialize(unsigned int width);
EXPORT_SYMBOL(ffe_initialize);
static void ffe_generate(unsigned int width, unsigned int height, void *vbuf);
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
static const u8 bar[8][3] = {
	COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK
};

static void ffe_initialize(unsigned int width)
{
	int i;
	u8 *p;

	for (i = 0; i < 16; i++) {
		int start = i * width / 8;
		int end = (i+1) * width / 8;
		int j;

		p = &raw[start * 3];

		for (j = start; j < end; j++) {
			memcpy(p, &bar[i % 8][0], 3);
			p += 3;
		}
	}
}

static void ffe_generate(unsigned int width, unsigned int height, void *vbuf)
{
	int size, i;
	u8 *start;

	if (!vbuf) {
		pr_err("%s: buffer error..\n", __func__);
		return;
	}

	size = width * 3;
	start = &raw[(count % width) * 3];

	for (i = 0; i < height; i++)
		memcpy(vbuf + i * size, start, size);

	count++;
}

static int __init ffe_init(void)
{
	pr_info("%s\n", __func__);
	return 0;
}


static void __exit ffe_exit(void)
{
	pr_info("%s\n", __func__);
}


module_init(ffe_init);
module_exit(ffe_exit);
