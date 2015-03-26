/*
 * Copyright (C) 2015 Johannes Thumshirn <morbidrsa@gmail.com>
 *
 * reflect is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

static unsigned long buffer_size = 1024;

struct buffer {
	char *data;
	unsigned long buffer_size;
	size_t len;
	struct mutex lock;
	wait_queue_head_t read_queue;
};

static struct buffer *buffer_alloc(unsigned long size)
{
	struct buffer *buf;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return NULL;


	buf->data = kzalloc(size, GFP_KERNEL);
	if (!buf->data)
		goto out_free;

	init_waitqueue_head(&buf->read_queue);
	mutex_init(&buf->lock);
	buf->buffer_size = size;

	return buf;

out_free:
	kfree(buf);
	return NULL;
}

static void buffer_free(struct buffer *buf)
{
	kfree(buf->data);
	kfree(buf);
}

static ssize_t reflect_write(struct file *file, const char __user *in,
			size_t size, loff_t *off)
{
	struct buffer *buf = file->private_data;
	int ret;

	if (size > buf->buffer_size) {
		ret = -EFBIG;
		goto out;
	}

	ret = mutex_lock_interruptible(&buf->lock);
	if (ret) {
		ret = -ERESTARTSYS;
		goto out;
	}

	ret = copy_from_user(buf->data, in, size);
	if (ret)
		goto out_unlock;

	buf->len = size;

	wake_up_interruptible(&buf->read_queue);

	ret = size;
out_unlock:
	mutex_unlock(&buf->lock);
out:
	return ret;

}

static ssize_t reflect_read(struct file *file, char __user *out,
			size_t size, loff_t *off)
{
	struct buffer *buf = file->private_data;
	int ret;

	while (!buf->len) {
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}

		ret = wait_event_interruptible(buf->read_queue, buf->len != 0);
		if (ret) {
			ret = -ERESTARTSYS;
			goto out;
		}
	}

	size = min(buf->len, size);
	ret = mutex_lock_interruptible(&buf->lock);
	if (ret) {
		ret = -ERESTARTSYS;
		goto out;
	}

	ret = copy_to_user(out, buf->data, size);
	if (ret) {
		ret = -EFAULT;
		goto out_unlock;
	}

	buf->len -= size;
	ret = size;

out_unlock:
	mutex_unlock(&buf->lock);
out:
	return ret;
}

static int reflect_open(struct inode *inode, struct file *file)
{
	file->private_data = buffer_alloc(buffer_size);

	if (!file->private_data)
		return -ENOMEM;

	return 0;
}

static int reflect_release(struct inode *inode, struct file *file)
{
	struct buffer *buf = file->private_data;

	buffer_free(buf);

	return 0;
}

static struct file_operations reflect_fops = {
	.open = reflect_open,
	.release = reflect_release,
	.read = reflect_read,
	.write = reflect_write,
	.llseek = noop_llseek,
};

static struct miscdevice reflect_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = KBUILD_MODNAME,
	.fops = &reflect_fops,
};

static int __init reflect_init(void)
{
	int ret;

	if (!buffer_size)
		return -1;

	ret = misc_register(&reflect_misc_device);
	pr_info("reflect device has been registered\n");

	return 0;
}
module_init(reflect_init);

static void __exit reflect_exit(void)
{
	misc_deregister(&reflect_misc_device);

	pr_info("reflect device has been unregistered\n");
}
module_exit(reflect_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Johannes Thumshirn <morbidrsa@gmail.com>");
MODULE_DESCRIPTION("In-kernel reflector");
