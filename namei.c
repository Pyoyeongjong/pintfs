#include <linux/kernel.h>
#include <linux/module.h>
#include "pintfs.h"
#include <linux/types.h>

static int pintfs_create(struct inode *dir, struct dentry* dentry, umode_t mode, bool excl)
{
	return 0;
}

const struct inode_operations pintfs_dir_inode_ops = {
	.create = pintfs_create,
};
