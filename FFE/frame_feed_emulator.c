// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>

#define MAX_WIDTH			3840
#define MAX_HEIGHT			2160
#define FRAME_COUNT			120
#define FRAME_RATE			60

MODULE_LICENSE("GPL");

struct videodata {
	unsigned int width, height, pixelsize;
} vdata;

struct frame {
	u8 *data;
	struct frame *next;
} *head, *p;

unsigned long ssize = 2 * MAX_WIDTH * MAX_HEIGHT;
static struct task_struct *ffe_thread;

static void ffe_generate(unsigned int width, void *vbuf);
EXPORT_SYMBOL(ffe_generate);

static void ffe_generate(unsigned int width, void *vbuf)
{
	int i, j;
	int fract = MAX_WIDTH / width;

	if (!vbuf) {
		pr_err("%s: buffer error..\n", __func__);
		return;
	}

	for (i = 0; i < MAX_HEIGHT; i += fract) {
		for (j = 0; j < (MAX_WIDTH << 1); j += (fract << 2)) {
			memcpy(vbuf, p->data + (i * (MAX_WIDTH << 1) + j), 4);
			vbuf += 4;
		}
	}

	p = p->next;
}

int thread_function(void *data)
{
	while (!kthread_should_stop()) {
		p = p->next;
		schedule_timeout_interruptible(msecs_to_jiffies(1000/FRAME_RATE));
	}

	pr_info("%s: exit\n", __func__);
	return 0;
}

static int __init ffe_init(void)
{
	int i;
	struct file *file = NULL;
	mm_segment_t fs = get_fs();
	loff_t pos = 0;

	pr_info("%s\n", __func__);

	p = head = vmalloc(sizeof(struct frame));
	p->data = vmalloc(ssize);
	for (i = 1; i < FRAME_COUNT; i++) {
		p = p->next = vmalloc(sizeof(struct frame));
		p->data = vmalloc(ssize);
	}
	p->next = head;

	set_fs(KERNEL_DS);
	file = filp_open("sample/video_3840x2160.yuv", O_RDONLY, 0);
	set_fs(fs);
	if (!IS_ERR(file)) {
		for (p = head; p->next != head; p = p->next) {
			set_fs(KERNEL_DS);
			vfs_read(file, p->data, ssize, &pos);
			set_fs(fs);
		}
		set_fs(KERNEL_DS);
		vfs_read(file, p->data, ssize, &pos);
		filp_close(file, NULL);
		set_fs(fs);
	}

	ffe_thread = kthread_run(thread_function, NULL, "FFE Thread");
	return 0;
}


static void __exit ffe_exit(void)
{
	struct frame *q;

	pr_info("%s\n", __func__);
	kthread_stop(ffe_thread);

	if (head) {
		p = head->next;
		while (p != head) {
			q = p;
			p = p->next;
			vfree(q->data);
			vfree(q);
		}
	vfree(head->data);
	vfree(head);
	}
}


module_init(ffe_init);
module_exit(ffe_exit);
