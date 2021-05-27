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
#define FPS_DEFAULT			25

MODULE_LICENSE("GPL");

int FRAME_RATE = FPS_DEFAULT;
EXPORT_SYMBOL(FRAME_RATE);

u8 *V_BUF = NULL;
EXPORT_SYMBOL(V_BUF);

bool I_FLAG = false;
EXPORT_SYMBOL(I_FLAG);

struct videodata {
	unsigned int width, height, pixelsize;
} vdata;

struct frame {
	u8 *data;
	struct frame *next;
} *head, *p;

unsigned long ssize = 2 * MAX_WIDTH * MAX_HEIGHT;
static struct task_struct *ffe_thread;

int ffe_thread_function(void *data)
{
	unsigned long jf = jiffies;
	unsigned long timeout;

	while(!kthread_should_stop()) {
		I_FLAG = false;
		V_BUF = p->data;
		I_FLAG = true;
		jf = jiffies - jf;
		timeout = msecs_to_jiffies((1000 / FRAME_RATE) - jf);
		schedule_timeout_interruptible(timeout);
		jf = jiffies;
		p = p->next;
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
		for(p = head; p->next != head; p = p->next) {
			set_fs(KERNEL_DS);
			vfs_read(file, p->data, ssize, &pos);
			set_fs(fs);
		}
		set_fs(KERNEL_DS);
		vfs_read(file, p->data, ssize, &pos);
		filp_close(file, NULL);
		set_fs(fs);
	}

	p = head;
	ffe_thread = kthread_run(ffe_thread_function, NULL, "FFE Thread");
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
