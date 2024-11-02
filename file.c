#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/uaccess.h>
#include "pintfs.h"
#define DEBUG 1

static bool pintfs_need_file_extension(struct inode *inode, int index){
	return PINTFS_I(inode)->i_data[index] == 0;
}

static int pintfs_expand_file(struct inode *inode, int index)
{
	
	struct super_block *sb = inode->i_sb;
	struct pintfs_inode_info *pii;
	int block_no;

	if(DEBUG)
		printk("pintfs - expand_file\n");

	pii = PINTFS_I(inode);
	if(pii->i_data[index] > 0)
		return 0;

	block_no = pintfs_empty_block(sb);
	
	if(block_no < 0)
		return -ENOMEM;

	pii->i_data[index] = block_no;
	set_bitmap(sb, PINTFS_BLOCK_BITMAP_BLOCK, block_no, 1);
	// when inode changes, write immediately!
	pintfs_write_inode(sb, inode);
	mark_inode_dirty(inode);
	return block_no;
}

static ssize_t pintfs_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct buffer_head *bh;
	struct inode *inode = filp->f_inode;
	char *start;
	struct super_block *sb;
	int index, offset, bytes_to_read, bytes_read, block_read;

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
	start = buf;

	printk("r : readblock\n");	
	index = *ppos / PINTFS_BLOCK_SIZE;
	offset = *ppos % PINTFS_BLOCK_SIZE;
	
	bytes_to_read = min(count, (size_t)(inode->i_size - *ppos));
	bytes_read = 0;

	printk("r : copy to user\n");
	while(bytes_read < bytes_to_read)
	{
		bh = pintfs_sb_bread_file(inode, index);	
		printk("DATA block content: %.20s\n", bh->b_data);
		if(!bh){
			printk(" here? 1");
			return -EIO;
		}
		block_read = min((PINTFS_BLOCK_SIZE - offset), bytes_to_read);
		if(copy_to_user(start + bytes_read, bh->b_data + offset, block_read)){
			printk(" here? 22");
			brelse(bh);
			return -EIO;
		}

		brelse(bh);
		bytes_read += block_read;
		*ppos += block_read;

		index++;
		offset = 0;
	}
	
	if (DEBUG)
		printk("pintfs - read %d bytes\n", bytes_read);

	return bytes_read;
}

static ssize_t pintfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = filp->f_inode;
	struct super_block *sb;
	struct buffer_head *bh;
	int index, offset, bytes_to_write, bytes_written, block_write;

	if(DEBUG)
		printk("pintfs - file write at ppos=%lld\n",*ppos);

	if(!inode)
		return -EINVAL;

	if(!(S_ISREG(inode->i_mode)))
		return -EINVAL;

	if(*ppos > inode->i_size || count < 0)
		return 0;

	sb = inode->i_sb;
	index = *ppos / PINTFS_BLOCK_SIZE;
	offset = *ppos % PINTFS_BLOCK_SIZE;

	if (DEBUG)
		printk("file write - index=%d, offset=%d\n",index, offset);

	bytes_to_write = count;
	bytes_written = 0;

	while(bytes_written < bytes_to_write)
	{
		if(pintfs_need_file_extension(inode, index)){
			if(pintfs_expand_file(inode, index) < 0){
				printk("w : failed to exted file\n");
				break;
			}
		}

		bh = pintfs_sb_bread_file(inode, index);
		if(!bh)
			return -EIO;

		block_write = min(PINTFS_BLOCK_SIZE - offset, bytes_to_write);
		if(copy_from_user(bh->b_data + offset, buf + bytes_written, block_write)){
			brelse(bh);
			return -EIO;
		}

		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
		brelse(bh);

		*ppos += block_write;
		bytes_written += block_write;

		if(*ppos > inode->i_size)
			inode->i_size = *ppos;
		
		index++;
		offset = 0;
	}

	pintfs_write_inode(inode->i_sb, inode);
	if (DEBUG)
		printk("pintfs - write %d bytes\n", bytes_written);
	return bytes_written;
}

/*
   FILE_OPERATIONS
*/
const struct file_operations pintfs_file_ops = {
	.read = pintfs_read,
	.write = pintfs_write,
};


