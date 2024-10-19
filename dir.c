#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "pintfs.h"

#define DEBUG 1

int pintfs_emptry_dir(struct inode *inode)
{
	struct buffer_head *bh;
	struct pintfs_dir_entry *pde
	int i, num_dirs;

	bh = sb_bread(inode->i_sb, PINTFS_I(inode->i_ino)->i_data[0]);
	if(!bh)
		return -EINVAL;

	num_dirs = PINTFS_BLOCK_SIZE / sizeof(pintfs_dir_entry);

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

static inline unsigned pintfs_setctx(unsigned long cur_pos,
        unsigned long aug_pos, unsigned long blocksize)
{
    cur_pos += aug_pos;

	unsigned next_pos = cur_pos + aug_pos;
    if (cur_pos % blocksize > next_pos % blocksize && next_pos % blocksize != 0) {
        cur_pos = (cur_pos / blocksize + 1) * blocksize;
    }

    return cur_pos;
}
/*
   pintfs_readdir - 중단된다면 중간 ctx->pos부터 탐색이 가능하게 만들어야 한다!
*/
static int 
pintfs_readdir(struct file *filp, struct dir_context *ctx)
{	
	struct inode *i = file_inode(filp);
	struct pintfs_inode_info *pi = PINTFS_I(i);
	struct pintfs_super_block *psb = PINTFS_SB(i->i_sb)->s_es;
	int num_dirs;
	struct pintfs_dir_entry *de;
	struct buffer_head *bh;
	unsigned long offset; // this is for iterating
	unsigned long blocksize;
	unsigned long nblocks;
	unsigned long block_no;

	if(DEBUG)
		printk("pintfs - readdir\n");
	
	blocksize = psb->block_size;
	nblocks = i->i_blocks;
	block_no = ctx->pos >> psb->blocksize_bits; 

	for( ; block_no < nblocks; block_no++){
		bh = sb_bread(i->i_sb, pi->i_data[block_no]);
		if(!bh){
			return -EIO;
		}
		offset = ctx->pos & (blocksize - 1);	
		while(offset < blocksize){
			if(offset + sizeof(struct pintfs_dir_entry) > blocksize)
				break;
			de = (struct pintfs_dir_entry *)(bh->b_data + offset);
			if(de->inode_number){
				if(!dir_emit(ctx, de->name, strnlen(de->name, MAX_NAME_SIZE), de->inode_number,
							DT_UNKNOWN)){
					brelse(bh);
					return -EINVAL;
				}
			}
			offset += sizeof(struct pintfs_dir_entry);
			ctx->pos += pintfs_setctx(ctx->pos, sizeof(struct pintfs_dir_entry), blocksize);
		}			

		brelse(bh);
	}
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
