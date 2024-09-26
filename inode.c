#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>

#include "pintfs.h"

#define DEBUG 1

/* 
   pintfs_empty_inode
   inode bitmap을 탐색해 비어있는 가장 작은 ino를 return
*/

static int pintfs_empty_inode(struct super_block *sb)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	struct buffer_head *bh;
	int inode_bitmap_bno; /* inode bitmap block number */
	char inode_bitmap[PINTFS_INODE_BITMAP_SIZE];
	sbi = PINTFS_SB(sb);
	if(!sbi)
		return -1;
	psb = sb->s_es;
	inode_bitmap_bno = psb->inode_bitmap_block;

	bh = sb_bread(sb, inode_bitmap_bno);
	if(!bh)
		return -1;

	memcpy(inode_bitmap, bh->b_data, PINTFS_INODE_BITMAP_SIZE);

	for(int i=PINTFS_GOOD_FIRST_INO; i<PINTFS_INODE_BITMAP_SIZE; i++){
		if(inode_bitmap[i] == 0)
			return i;
	}

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

	new_inum = pintfs_empty_inode(sb);

	if(new_inum == -1){
		printk("pintfs - inode table is full.\n");
		return NULL;
	}

	inode_init_owner(inode, dir, mode);
	struct timespec64 current_time = CURRENT_TIME;

	block.i_mode = mode;
	block.i_uid = i_uid_read(inode);
	block.i_size = 0;
	block.i_time = current_time;

	// Write pintfs_inode in disk!
	// TODO: Implement this!
	pintfs_write_inode(sb, new_inum, &block);

	inode->i_ino = new_inum;
	inode->i_ctime = inode-> i_mtime = inode->i_atione = current_time;
	insert_inode_hash(inode);
	return inode;
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


