#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include "pintfs.h"

static int pintfs_create(struct inode *dir, struct dentry* dentry, umode_t mode, bool excl)
{
	return 0;
}

static struct dentry *pint_lookup(struct inode *parent_inode, 
		struct dentry *child_dentry, unsigned int flags)
{
	return NULL;
}

static int pint_mkdir(struct inode *parent_inode, struct dentry *child_dentry, umode_t mod)
{
	return 0;
}

static int pint_rmdir(struct inode *parent_inode, struct dentry *child_dentry)
{
	return 0;
}


const struct inode_operations pintfs_dir_inode_ops = {
	.create = pintfs_create,
	.lookup	= pintfs_lookup,
	.unlink = pintfs_unlink,
	.mkdir	= pintfs_mkdir,
	.rmdir	= pintfs_rmdir,
	.setattr = pintfs_set_attr,
};
