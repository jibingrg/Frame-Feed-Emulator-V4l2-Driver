// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

struct videodata {
	unsigned int width, height, pixelsize;
	unsigned long size;
} vdata;

struct file *file;
mm_segment_t fs;
loff_t pos;
int count, fcount;
bool flag = true;

static void ffe_initialize(unsigned int width, unsigned int height, unsigned int pixelsize);
EXPORT_SYMBOL(ffe_initialize);
static void ffe_generate(void *vbuf);
EXPORT_SYMBOL(ffe_generate);

static void ffe_initialize(unsigned int width, unsigned int height, unsigned int pixelsize)
{
	vdata.width = width;
	vdata.height = height;
	vdata.pixelsize = pixelsize;
	vdata.size = vdata.width * vdata.height * vdata.pixelsize;
	flag = false;
	pos = count = 0;

	if (pixelsize == 2) {
		switch (width) {
		case 480:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_480x270.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 900;
			if (IS_ERR(file))
				flag = true;
			break;
		case 640:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_640x360.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 900;
			if (IS_ERR(file))
				flag = true;
			break;
		case 1280:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_sample_1280x720_5s.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		case 1920:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_sample_1920x1080_5s.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		case 2560:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_sample_2560x1440_5s.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		case 3840:
			set_fs(KERNEL_DS);
			file = filp_open("sample/yuyv_sample_3840x2160_5s.yuv", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		default:
			flag = true;
			break;
		}
	} else if (pixelsize == 3) {
		switch (width) {
		case 480:
			set_fs(KERNEL_DS);
			file = filp_open("sample/rgb_480x270.rgb", O_RDONLY, 0);
			set_fs(fs);
			fcount = 900;
			if (IS_ERR(file))
				flag = true;
			break;
		case 640:
			set_fs(KERNEL_DS);
			file = filp_open("sample/rgb_640x360.rgb", O_RDONLY, 0);
			set_fs(fs);
			fcount = 900;
			if (IS_ERR(file))
				flag = true;
			break;
		case 1280:
			set_fs(KERNEL_DS);
			file = filp_open("sample/sample_1280x720_5s.rgb", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		case 1920:
			set_fs(KERNEL_DS);
			file = filp_open("sample/sample_1920x1080_5s.rgb", O_RDONLY, 0);
			set_fs(fs);
			fcount = 120;
			if (IS_ERR(file))
				flag = true;
			break;
		default:
			flag = true;
			break;
		}
	} else {
		flag = true;
	}
}

static void ffe_generate(void *vbuf)
{
	int i;
	u8 color = 0;

	if (!vbuf) {
		pr_err("%s: buffer error..\n", __func__);
		return;
	}

	if (flag) {
		for (i = 0; i < vdata.size; i++)
			memcpy(vbuf + i, &color, 1);
	} else {
		set_fs(KERNEL_DS);
		vfs_read(file, vbuf, vdata.size, &pos);
		set_fs(fs);
		count++;
		if (count >= fcount) {
			pos = 0;
			count = 0;
		}
	}
}

static int __init ffe_init(void)
{
	pr_info("%s\n", __func__);
	fs = get_fs();
	return 0;
}


static void __exit ffe_exit(void)
{
	pr_info("%s\n", __func__);
	if (!flag) {
		set_fs(KERNEL_DS);
		filp_close(file, NULL);
		set_fs(fs);
	}
}


module_init(ffe_init);
module_exit(ffe_exit);
