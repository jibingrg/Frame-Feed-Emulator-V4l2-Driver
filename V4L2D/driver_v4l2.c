// SPDX-License-Identifier: GPL

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-vmalloc.h>

#include "driver_v4l2.h"

MODULE_DESCRIPTION("V4L2 Driver with FFE");
MODULE_LICENSE("GPL");


static void p_release(struct device *dev)
{
	dev_info(dev, "%s: release", __func__);
}

static struct platform_device p_device = {
	.name		= KBUILD_MODNAME,
	.id		= PLATFORM_DEVID_NONE,
	.dev		= {
		.release		= p_release,
	},
};

struct ffe_buffer {
	struct vb2_buffer		vb;
	struct list_head		list;
	struct v4l2_buffer		v4l2_buf;
};

struct ffe_dmaq {
	struct list_head		active;
	struct task_struct		*kthread;
	wait_queue_head_t		wq;
};

struct dev_data {
	struct platform_device		*pdev;
	struct v4l2_device		v4l2_dev;
	struct video_device		vdev;
	struct mutex			mutex;
	struct vb2_queue		queue;
	struct ffe_dmaq			vidq;
	u32				pixelformat;
	struct v4l2_fract		timeperframe;
	spinlock_t			s_lock;
	int				input;
	unsigned int			f_count;
	unsigned int			width, height, pixelsize;
};

static int thread_function(void *data)
{
	struct dev_data *dev = data;
	struct ffe_dmaq *q = &dev->vidq;
	struct ffe_buffer *buf;
	unsigned long flags = 0;
	unsigned long size = F_DATA.width * F_DATA.height << 1;
	void *vbuf;

	v4l2_info(&dev->v4l2_dev, "%s: k_thread\n", __func__);

	while (!kthread_should_stop()) {
		DECLARE_WAITQUEUE(wait, current);

		if (!I_FLAG)
			continue;

		add_wait_queue(&q->wq, &wait);
		spin_lock_irqsave(&dev->s_lock, flags);

		if (list_empty(&q->active)) {
			v4l2_err(&dev->v4l2_dev, "%s: No active queue\n", __func__);
			spin_unlock_irqrestore(&dev->s_lock, flags);
		} else {
			buf = list_entry(q->active.next, struct ffe_buffer, list);
			vbuf = vb2_plane_vaddr(&buf->vb, 0);
			list_del(&buf->list);
			spin_unlock_irqrestore(&dev->s_lock, flags);
			v4l2_info(&dev->v4l2_dev, "frame rate = %d, frame number = %d\n", F_DATA.framerate, V_BUF->frame_no);
			memcpy(vbuf, V_BUF->data, size);
			buf->v4l2_buf.field = V4L2_FIELD_INTERLACED;
			buf->v4l2_buf.sequence = dev->f_count++;
			vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
		}

		remove_wait_queue(&q->wq, &wait);
		I_FLAG = false;
	}
	v4l2_info(&dev->v4l2_dev, "%s: k_thread exit\n", __func__);
	return 0;
}

static int queue_setup(struct vb2_queue *vq, unsigned int *nbuffers, unsigned int *nplanes, unsigned int sizes[], struct device *alloc_ctxs[])
{
	struct dev_data *dev = vb2_get_drv_priv(vq);
	unsigned long size = F_DATA.width * F_DATA.height << 1;

	if (!V_BUF) {
		v4l2_err(&dev->v4l2_dev, "%s: FFE is not initialized..\n", __func__);
		return -ENODATA;
	}

	*nplanes = 1;
	sizes[0] = size;
	v4l2_info(&dev->v4l2_dev, "%s: width = %d, height = %d, size = %ld\n", __func__, dev->width, dev->height, size);
	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct dev_data *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct ffe_buffer *buf = container_of(vb, struct ffe_buffer, vb);
	unsigned long size = dev->width * dev->height * dev->pixelsize;

	if (vb2_plane_size(vb, 0) < size) {
		v4l2_err(&dev->v4l2_dev, "%s: vb2 plane size is less than required..\n", __func__);
		return -EINVAL;
	}

	vb2_set_plane_payload(&buf->vb, 0, size);
	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct dev_data *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct ffe_buffer *buf = container_of(vb, struct ffe_buffer, vb);
	struct ffe_dmaq *vidq = &dev->vidq;
	unsigned long flags = 0;

	spin_lock_irqsave(&dev->s_lock, flags);
	list_add_tail(&buf->list, &vidq->active);
	spin_unlock_irqrestore(&dev->s_lock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct dev_data *dev = vb2_get_drv_priv(vq);
	struct ffe_dmaq *q = &dev->vidq;

	dev->f_count = 0;
	q->kthread = kthread_run(thread_function, dev, "%s", dev->v4l2_dev.name);

	if (IS_ERR(q->kthread)) {
		v4l2_err(&dev->v4l2_dev, "%s: kernel_thread() failed..\n", __func__);
		return PTR_ERR(q->kthread);
	}
	wake_up_interruptible(&q->wq);
	return 0;
}

static void stop_streaming(struct vb2_queue *vq)
{
	struct dev_data *dev = vb2_get_drv_priv(vq);
	struct ffe_dmaq *q = &dev->vidq;

	if (q->kthread) {
		kthread_stop(q->kthread);
		q->kthread = NULL;
	}

	while (!list_empty(&q->active)) {
		struct ffe_buffer *buf;

		buf = list_entry(q->active.next, struct ffe_buffer, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
}

static const struct vb2_ops ffe_qops = {
	.queue_setup			= queue_setup,
	.buf_prepare			= buffer_prepare,
	.buf_queue			= buffer_queue,
	.start_streaming		= start_streaming,
	.stop_streaming		= stop_streaming,
	.wait_prepare			= vb2_ops_wait_prepare,
	.wait_finish			= vb2_ops_wait_finish,
};

static int vidioc_querycap(struct file *file, void  *priv, struct v4l2_capability *cap)
{
	struct dev_data *dev = video_drvdata(file);

	strcpy(cap->driver, KBUILD_MODNAME);
	strcpy(cap->card, KBUILD_MODNAME);
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", dev->v4l2_dev.name);
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;

	strscpy(f->description, "YUYV 4:2:2", sizeof(f->description));
	f->pixelformat = V4L2_PIX_FMT_YUYV;

	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dev_data *dev = video_drvdata(file);

	f->fmt.pix.width = dev->width = F_DATA.width;
	f->fmt.pix.height = dev->height = F_DATA.height;
	f->fmt.pix.field = V4L2_FIELD_INTERLACED;
	f->fmt.pix.pixelformat = dev->pixelformat;
	f->fmt.pix.bytesperline = f->fmt.pix.width * dev->pixelsize;
	f->fmt.pix.sizeimage = f->fmt.pix.height * f->fmt.pix.bytesperline;
	f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
	return 0;
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dev_data *dev = video_drvdata(file);

	if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		v4l2_err(&dev->v4l2_dev, "%s: Unknown format..\n", __func__);

	dev->input = 0;
	f->fmt.pix.pixelformat = dev->pixelformat = V4L2_PIX_FMT_YUYV;
	dev->pixelsize = 2;
	f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;

	if ((f->fmt.pix.width != F_DATA.width) || (f->fmt.pix.height != F_DATA.height))
		v4l2_info(&dev->v4l2_dev, "%s pixel width and height changed to default\n", __func__);

	f->fmt.pix.width = dev->width = F_DATA.width;
	f->fmt.pix.height = dev->height = F_DATA.height;
	f->fmt.pix.field = V4L2_FIELD_INTERLACED;
	f->fmt.pix.bytesperline = dev->width * dev->pixelsize;
	f->fmt.pix.sizeimage = dev->height * f->fmt.pix.bytesperline;
	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct dev_data *dev = video_drvdata(file);
	struct vb2_queue *q = &dev->queue;

	vidioc_try_fmt_vid_cap(file, priv, f);

	if (vb2_is_busy(q)) {
		v4l2_err(&dev->v4l2_dev, "%s device busy..\n", __func__);
		return -EBUSY;
	}

	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct v4l2_frmsize_discrete size = { F_DATA.width, F_DATA.height};

	if (fsize->index)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete = size;
	return 0;
}

static int vidioc_enum_frameintervals(struct file *file, void *priv, struct v4l2_frmivalenum *fival)
{
	if (fival->index)
		return -EINVAL;

	if (fival->pixel_format != V4L2_PIX_FMT_YUYV)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete = (struct v4l2_fract) {1, F_DATA.framerate};
	return 0;
}

static int vidioc_enum_input(struct file *file, void *priv, struct v4l2_input *inp)
{
	if (inp->index)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	sprintf(inp->name, "Camera %u", inp->index);
	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct dev_data *dev = video_drvdata(file);

	*i = dev->input;
	return 0;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct dev_data *dev = video_drvdata(file);

	if (i)
		return -EINVAL;

	dev->input = 0;
	return 0;
}

static int vidioc_g_parm(struct file *file, void *priv, struct v4l2_streamparm *parm)
{
	struct v4l2_fract tpf;
	struct dev_data *dev = video_drvdata(file);

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	tpf = (struct v4l2_fract) {1, F_DATA.framerate};
	parm->parm.capture.capability   = V4L2_CAP_TIMEPERFRAME;
	parm->parm.capture.timeperframe = dev->timeperframe = tpf;
	parm->parm.capture.readbuffers  = 1;
	return 0;
}

static int vidioc_s_parm(struct file *file, void *priv, struct v4l2_streamparm *parm)
{
	struct v4l2_fract tpf;
	struct dev_data *dev = video_drvdata(file);

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	tpf = (struct v4l2_fract) {1, F_DATA.framerate};

	dev->timeperframe = parm->parm.capture.timeperframe = tpf;
	parm->parm.capture.readbuffers = 1;
	return 0;
}

static const struct v4l2_ioctl_ops ffe_ioctl_ops = {
	.vidioc_querycap		= vidioc_querycap,
	.vidioc_enum_fmt_vid_cap	= vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= vidioc_s_fmt_vid_cap,
	.vidioc_enum_framesizes		= vidioc_enum_framesizes,
	.vidioc_enum_frameintervals	= vidioc_enum_frameintervals,
	.vidioc_g_input			= vidioc_g_input,
	.vidioc_s_input			= vidioc_s_input,
	.vidioc_g_parm			= vidioc_g_parm,
	.vidioc_s_parm			= vidioc_s_parm,
	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,
	.vidioc_enum_input		= vidioc_enum_input,
	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

static const struct v4l2_file_operations ffe_fops = {
	.owner				= THIS_MODULE,
	.open				= v4l2_fh_open,
	.release			= vb2_fop_release,
	.read				= vb2_fop_read,
	.poll				= vb2_fop_poll,
	.unlocked_ioctl			= video_ioctl2,
	.mmap				= vb2_fop_mmap,
};

static int p_probe(struct platform_device *pdev)
{
	struct dev_data *dev;
	struct video_device *vdev;
	struct vb2_queue *q;
	int ret;

	dev_info(&pdev->dev, "%s: probe function\n", __func__);
	dev = devm_kzalloc(&pdev->dev, sizeof(struct dev_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	dev->pdev = pdev;
	dev_set_drvdata(&pdev->dev, dev);

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "%s: v4l2 device registration failed..\n", __func__);
		return ret;
	}

	dev->pixelformat = V4L2_PIX_FMT_YUYV;
	dev->timeperframe = (struct v4l2_fract) {1, F_DATA.framerate};
	dev->width = F_DATA.width;
	dev->height = F_DATA.height;
	dev->pixelsize = 2;
	dev->input = 0;

	spin_lock_init(&dev->s_lock);
	q = &dev->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = dev;
	q->buf_struct_size = sizeof(struct ffe_buffer);
	q->ops = &ffe_qops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	ret = vb2_queue_init(q);
	if (ret) {
		dev_err(&pdev->dev, "%s: vb2 queue init failed..\n", __func__);
		v4l2_device_unregister(&dev->v4l2_dev);
		return ret;
	}

	mutex_init(&dev->mutex);
	INIT_LIST_HEAD(&dev->vidq.active);
	init_waitqueue_head(&dev->vidq.wq);

	vdev = &dev->vdev;
	strscpy(vdev->name, KBUILD_MODNAME, sizeof(vdev->name));
	vdev->release = video_device_release_empty;
	vdev->fops = &ffe_fops;
	vdev->ioctl_ops = &ffe_ioctl_ops;
	vdev->v4l2_dev = &dev->v4l2_dev;
	vdev->queue = q;
	vdev->lock = &dev->mutex;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	video_set_drvdata(vdev, dev);

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: video device registration failed..\n", __func__);
		v4l2_device_unregister(&dev->v4l2_dev);
		video_device_release(&dev->vdev);
		return ret;
	}

	v4l2_info(&dev->v4l2_dev, "%s: V4L2 device registered as %s\n", __func__, video_device_node_name(vdev));
	return 0;
}

static int p_remove(struct platform_device *pdev)
{
	struct dev_data *dev;

	dev_info(&pdev->dev, "%s: remove function\n", __func__);
	dev = platform_get_drvdata(pdev);
	v4l2_info(&dev->v4l2_dev, "%s: unregistering %s\n", __func__, video_device_node_name(&dev->vdev));
	video_unregister_device(&dev->vdev);
	v4l2_device_unregister(&dev->v4l2_dev);
	return 0;
}

static struct platform_driver p_driver = {
	.probe = p_probe,
	.remove = p_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
	},
};

static int __init v4l2_init(void)
{
	int ret;

	pr_info("%s: Inserting V4L2 driver module\n", __func__);

	ret = platform_device_register(&p_device);
	if (ret) {
		pr_err("%s: platform device, %s registration failed..\n", __func__, p_device.name);
		return ret;
	}

	ret = platform_driver_register(&p_driver);
	if (ret) {
		pr_err("%s: platform driver, %s registration failed..\n", __func__, p_driver.driver.name);
		platform_device_unregister(&p_device);
	}
	return ret;
}

static void __exit v4l2_exit(void)
{
	pr_info("%s: Removing V4L2 driver module\n", __func__);

	platform_driver_unregister(&p_driver);
	platform_device_unregister(&p_device);
}

module_init(v4l2_init);
module_exit(v4l2_exit);
