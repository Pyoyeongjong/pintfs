#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "pintfs.h"

#define DEBUG 1

int pintfs_empty_dir(struct inode *inode)
{
	struct buffer_head *bh;
	struct pintfs_dir_entry *pde;
	int i, num_dirs;

	bh = pintfs_sb_bread_dir(inode);
	if(!bh)
		return -EINVAL;

	num_dirs = PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);

	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));
		if(pde->inode_number){
			brelse(bh);
			return false;
		}
	}

	brelse(bh);
	return true;
	
}

/*
   pintfs_readdir - 중단된다면 중간 ctx->pos부터 탐색이 가능하게 만들어야 한다!
*/
static int pintfs_readdir(struct file *filp, struct dir_context *ctx)
{	
	struct inode *i;
	struct pintfs_super_block *psb = PINTFS_SB(i->i_sb)->s_es;
	int num_dirs, k;
	struct pintfs_dir_entry *de;
	struct buffer_head *bh;
	unsigned long offset; // this is for iterating
	unsigned long block_no = 0;

	if(DEBUG)
		printk("pintfs - readdir\n");
	

	i = file_inode(filp);
	bh = pintfs_sb_bread_dir(i);
	if(!bh){
		return -EIO;
	}
	num_dirs = NUM_DIRS;
	offset = 0;

	for(k=0; k<num_dirs; k++){
		de = (struct pintfs_dir_entry *)(bh->b_data + offset);
		if(de->inode_number){
			printk("readdir - inode_number=%d\n",de->inode_number);
			if(!dir_emit(ctx, de->name, strnlen(de->name, MAX_NAME_SIZE), de->inode_number,
						DT_UNKNOWN)){
				brelse(bh);
				return 0;
			}
		}
		ctx->pos += sizeof(struct pintfs_dir_entry);
		offset += sizeof(struct pintfs_dir_entry);
	}

	brelse(bh);

	if(DEBUG)
		printk("pintfs - readdir done\n");
	return 0;
}

/*
   DIRECTORY_OPERATIONS
*/
const struct file_operations pintfs_dir_ops = {
	.llseek	=	generic_file_llseek,
	.read	=	generic_read_dir,
	.iterate =	pintfs_readdir,
	.fsync	=	generic_file_fsync,
};
