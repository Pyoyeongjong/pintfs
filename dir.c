#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include "pintfs.h"

static ssize_t pintfs_read_dir(struct file *filp, char __user *buffer, size_t count, loff_t *offset)
{	
	return 0;
}

const struct file_operations pintfs_dir_ops = {
	.read = pintfs_read_dir,
};
