#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/buffer_head.h>

#include "pintfs.h"

int pintfs_empty_block(struct super_block *sb)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	struct buffer_head *bh;
	int block_bitmap_bno;
	char block_bitmap[PINTFS_BLOCK_BITMAP_SIZE];
	int result = -1, i;
	
	sbi = PINTFS_SB(sb);

	if(!sbi)
		return result;

	psb = sbi->s_es;
	block_bitmap_bno = psb->block_bitmap_block;

	bh = sb_bread(sb, block_bitmap_bno);
	if(!bh)
		return result;

	memcpy(block_bitmap, bh->b_data, PINTFS_BLOCK_BITMAP_SIZE);	
	for(i=PINTFS_FIRST_DATA_BLOCK; i<PINTFS_BLOCK_BITMAP_SIZE; i++){
		if(block_bitmap[i] == 0){
			result = i;
			break;
		}
	}
	
	brelse(bh);
	return result;
}
