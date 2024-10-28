#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/iversion.h>
#include <linux/uidgid.h>
#include <linux/buffer_head.h>

#include "pintfs.h"
#define DEBUG 1
int set_bitmap(struct super_block *sb, int bno, int no, int val)
{
	struct buffer_head *bh;
	struct pintfs_super_block *psb = PINTFS_SB(sb)->s_es;

	if(bno == psb->inode_bitmap_block && no >= psb->inodes_count)
		return -1;
	else if(bno == psb->block_bitmap_block && no >= psb->blocks_count)
		return -1;
	else
		return -EINVAL;

	bh = sb_bread(sb, bno);
	if(!bh)
		return -EINVAL;

	bh->b_data[no] = val;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	return 0;
}

/*
	pintfs_put_super - remove memory of super_block
*/
static void pintfs_put_super(struct super_block *sb)
{
	struct pintfs_sb_info *sbi = PINTFS_SB(sb);
	struct pintfs_super_block *ps = sbi->s_es;

	if (DEBUG)
		printk("pintfs - put_super\n");

	kfree(ps);
	kfree(sbi);
	sb->s_fs_info = NULL;
	if(DEBUG)
		printk("pintfs - put_super ok\n");
	return;
}

/*
	pintfs_statfs - superblock stats
*/
static int pintfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	if (DEBUG) 
		printk("pintfs - statfs\n");
	return 0;
}


/*
	SUPER_OPERATIONS
*/
const struct super_operations pintfs_super_ops = {
	.alloc_inode = pintfs_alloc_inode,
	.free_inode = pintfs_free_inode,
	.evict_inode = pintfs_evict_inode,
	.put_super = pintfs_put_super,	
	.statfs = pintfs_statfs,
};

/*
	pintfs_fill_super - Initialize Superblock in main memory
*/
static int pintfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	unsigned long sb_block = PINTFS_SUPER_BLOCK;		/* Default location */
	struct inode *root;
	long ret = -ENOMEM;
	struct buffer_head *bh;


	if (DEBUG)
		printk("pintfs - fill_super\n");

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if(!sbi)
		goto failed;
	
	// get pintfs_super_block!
	bh = sb_bread(sb, sb_block);
	if(!bh)
		goto failed_sbi;

	sb->s_fs_info = sbi;
	psb = (struct pintfs_super_block *) (((char *)bh->b_data));
	sbi->s_es = kzalloc(sizeof(struct pintfs_super_block), GFP_KERNEL);
	if(!sbi->s_es)
		goto failed_bh;
	memcpy(sbi->s_es, psb, sizeof(struct pintfs_super_block));
	sbi->s_first_ino = PINTFS_GOOD_FIRST_INO;
	sbi->s_inode_size = PINTFS_INODE_SIZE;

	sb->s_magic = psb->magic;
	sb->s_op = &pintfs_super_ops;

	root = pintfs_iget(sb, PINTFS_ROOT_INO);
	if(!root){
		ret = -ENOMEM;
		goto failed_s_es;
	}

	sb->s_blocksize = PINTFS_BLOCK_SIZE;
	// setting root directory
	sb->s_root = d_make_root(root);
	if(!sb->s_root) {
		ret = -ENOMEM;
		goto failed_inode;
	}

	brelse(bh);
	if(DEBUG)
		printk("pintfs - fill super ok!\n");
	return 0;

failed_inode:
	iput(root);
failed_s_es:
	kfree(sbi->s_es);
failed_bh:
	brelse(bh);
failed_sbi:
	sb->s_fs_info = NULL;
	kfree(sbi);
failed:
	printk("pintfs - fill super not ok!\n");
	return ret;
}

/*
   pintfs_mount - Register this filesystem in Linux and synchronize with Disk Image
*/
static struct dentry *pintfs_mount(struct file_system_type *fs_type, int flags,
		const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, pintfs_fill_super);
}

/*
	pintfs_type - type of filesystem
*/
static struct file_system_type pintfs_type = {
	.owner = THIS_MODULE, // for controlling ref_count to protect module wit mount and unmount
	.name = "pintfs",
	.mount = pintfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("pintfs");

/*
	init_pintfs, exit_pintfs - init, exit module
*/
static int __init init_pintfs(void)
{ // __init <- for being in initialize code section
	return register_filesystem(&pintfs_type);	
}

static void __exit exit_pintfs(void)
{
	unregister_filesystem(&pintfs_type);
}

module_init(init_pintfs);
module_exit(exit_pintfs);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pyo YeongJong");
MODULE_DESCRIPTION("Pintfs: A simple virtual filesystem");
