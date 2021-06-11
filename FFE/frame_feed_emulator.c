// SPDX-License-Identifier: GPL

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");

struct ffe_frame {
	int frame_no;
	u8 *data;
	struct ffe_frame *next;
} *head, *p;

struct ffe_data {
	int framerate;
	int framecount;
	int width;
	int height;
};

struct ffe_frame *V_BUF;
EXPORT_SYMBOL(V_BUF);

struct ffe_data F_DATA;
EXPORT_SYMBOL(F_DATA);

bool I_FLAG;
EXPORT_SYMBOL(I_FLAG);

dev_t device;
struct cdev *cdev;
char name[] = "FFE";
struct class *class;
static struct task_struct *ffe_thread;

int ffe_thread_function(void *data)
{
	unsigned long jf = jiffies;
	unsigned long timeout;

	pr_info("%s: k_thread\n", __func__);

	while (!kthread_should_stop()) {
		I_FLAG = false;
		V_BUF = p;
		I_FLAG = true;
		timeout = msecs_to_jiffies(1000 / F_DATA.framerate) - (jiffies - jf);
		schedule_timeout_interruptible(timeout);
		jf = jiffies;
		p = p->next;
	}

	pr_info("%s: k_thread exit\n", __func__);
	return 0;
}

int ffe_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int ffe_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long ffe_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i;
	struct ffe_frame *q;
	unsigned long size;

	if (cmd == 1) {
		p = head;
		ffe_thread = kthread_run(ffe_thread_function, NULL, "FFE Thread");
		return 0;
	}

	if (ffe_thread)
		kthread_stop(ffe_thread);

	copy_from_user(&F_DATA, (struct ffe_data *) arg, sizeof(struct ffe_data));
	pr_info("%s: frame rate = %d, frame count = %d\n", __func__, F_DATA.framerate, F_DATA.framecount);
	pr_info("%s: frame width = %d, frame height = %d\n", __func__, F_DATA.width, F_DATA.height);
	size = F_DATA.width * F_DATA.height << 1;

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

	p = head = vmalloc(sizeof(struct ffe_frame));
	p->frame_no = 0;
	p->data = vmalloc(size);
	for (i = 1; i < F_DATA.framecount; i++) {
		p->frame_no = i;
		p = p->next = vmalloc(sizeof(struct ffe_frame));
		p->data = vmalloc(size);
	}
	p = p->next = head;

	return 0;
}

ssize_t ffe_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	static int i;

	copy_from_user(p->data + (i * (F_DATA.width << 1)), buf, count);
	i++;
	if (i == F_DATA.height) {
		i = 0;
		p = p->next;
	}
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = ffe_open,
	.release = ffe_release,
	.write = ffe_write,
	.unlocked_ioctl = ffe_ioctl,
};

static int __init ffe_init(void)
{
	pr_info("%s: Inserting FFE\n", __func__);

	if (alloc_chrdev_region(&device, 0, 1, name) < 0) {
		pr_err("device registration failed..\n");
		return -1;
	}
	pr_info("Device: %s, Major number: %d\n", name, MAJOR(device));
	class = class_create(THIS_MODULE, name);
	device_create(class, NULL, device, NULL, name);
	cdev = cdev_alloc();
	cdev_init(cdev, &fops);
	cdev_add(cdev, device, 1);

	return 0;
}


static void __exit ffe_exit(void)
{
	struct ffe_frame *q;

	pr_info("%s: Removing FFE\n", __func__);

	if (ffe_thread)
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

	cdev_del(cdev);
	device_destroy(class, device);
	class_destroy(class);
	unregister_chrdev_region(device, 1);
}


module_init(ffe_init);
module_exit(ffe_exit);
