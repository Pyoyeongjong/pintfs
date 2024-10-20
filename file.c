#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "pintfs.h"
#define DEBUG 1

//TODO:
static ssize_t pint_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct buffer_head *bh;
	struct inode *inode = filp->f_inode;
	char *start;
	int inode_no;
	struct super_block *sb;

	if(DEBUG)
		printk("pintfs - file read - count: %zu ppos %Ld\n", count, *ppos);

	if(!inode){
		printk("pintfs - Problem with file inode\n");
		return -EINVAL;
	}

	if(!(S_ISREG(inode->i_mode))){
		printk("pintfs - not regular file\n");
		return -EINVAL;
	}

	if(*ppos > inode->i_size || count <= 0){
		printk("pintfs - attempting to write over the end of a file.\n");
		return 0;
	}
	
	sb = inode->i_sb;
	
	printk("r : readblock\n");	
	// inode get...
	bh = sb_bread(sb, PINTFS_I(inode)->i_data[0]);

	start = buf;

	return 0;
}

static ssize_t pint_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	return 0;
}

/*
   FILE_OPERATIONS
*/
const struct file_operations pintfs_file_ops = {
	.read = pint_read,
	.write = pint_write,
};


