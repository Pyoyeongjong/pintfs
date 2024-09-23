#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include "pintfs.h"
#include <linux/buffer_head.h>
#define DEBUG 1


static int pintfs_empty_inode(struct super_block *sb)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	struct buffer_head *bh;
	int inode_bitmap_bno;
	char inode_
	sbi = PINTFS_SB(sb);
	if(!sbi)
		return -1;
	psb = sb->s_es;
	inode_bitmap_bno = psb->inode_bitmap_block;

	bh = sb_bread(sb, inode_bitmap_bno);
	if(!bh)
		return -1;
	


}

static inode *pintfs_new_inode(const struct inode *dir, umode_t mode)
{
	struct pintfs_inode block;
	struct super_block *sb;
	struct inode *inode;
	int new_ino;

	if (DEBUG)
		printk("pintfs - new inode\n");
	
	if(!dir)
		return NULL;
	sb = dir->i_sb;

	inode = new_inode(sb);
	if(!inode)
		return NULL;

	
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

const struct inode_operations pintfs_inode_ops = {
	.lookup = pint_lookup,
	.mkdir = pint_mkdir,
	.rmdir = pint_rmdir,
};


