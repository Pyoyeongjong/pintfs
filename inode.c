#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/iversion.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/time64.h>
#include <linux/module.h>

#include "pintfs.h"

#define DEBUG 1


/*
	pintfs_write_inode - Write pintfs_inode in block device
*/
int pintfs_write_inode(struct super_block *sb, struct inode* inode)
{
	struct buffer_head *bh;
	struct pintfs_inode pinode;
	struct pintfs_inode_info *pii = PINTFS_I(inode);
	int inum = inode->i_ino;
	if(DEBUG)
		printk("pintfs - write_inode in block device (inum:%d)\n", inum);

	pinode.i_mode = inode->i_mode;
	pinode.i_uid = from_kuid(&init_user_ns, inode->i_uid);  // 변환 후 저장
	pinode.i_size = inode->i_size;
	pinode.i_time = inode->i_mtime.tv_sec;
	memcpy(pinode.i_block, pii->i_data, PINTFS_N_BLOCKS);
	pinode.i_blocks = inode->i_blocks;

	bh = sb_bread(sb, pintfs_get_blocknum(inum));
	if(!bh)
		return -ENOSPC;
	memcpy(bh->b_data + inum*sizeof(struct pintfs_inode), &pinode, sizeof(struct pintfs_inode));
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if(DEBUG)
		printk("pintfs - write_inode done (inum=%d)\n", inum);
	return PINTFS_INODE_SIZE;	
}

int pintfs_empty_inode(struct super_block *sb)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	struct buffer_head *bh;
	int inode_bitmap_bno; /* inode bitmap block number */
	char inode_bitmap[PINTFS_INODE_BITMAP_SIZE];
	int result = -1, i;
	
	if(DEBUG)
		printk("pintfs - pintfs_empty_inode\n");

	sbi = PINTFS_SB(sb);

	if(!sbi)
		return result;

	psb = sbi->s_es;
	inode_bitmap_bno = psb->inode_bitmap_block;

	bh = sb_bread(sb, inode_bitmap_bno);
	if(!bh)
		return result;

	memcpy(inode_bitmap, bh->b_data, PINTFS_INODE_BITMAP_SIZE);	
	printk("empty_inode = ");
	for(i=PINTFS_GOOD_FIRST_INO; i<PINTFS_INODE_BITMAP_SIZE; i++){
		printk("%d ",i);
		if(inode_bitmap[i] == 0){
			bh->b_data[i] = 1;
			mark_buffer_dirty(bh);
			sync_dirty_buffer(bh);
			result = i;
			break;
		}
	}
	
	brelse(bh);
	return result;
}



void pintfs_evict_inode(struct inode *inode)
{
	struct pintfs_inode_info *pii;
	struct super_block *sb;
	int ino, i;

	if (DEBUG)
		printk("pintfs - pintfs_evict_inode\n");

	pii = PINTFS_I(inode);
	ino = inode->i_ino;
	sb = inode->i_sb;

	for(i=0; i<PINTFS_N_BLOCKS; i++)
	{
		if(pii->i_data[i] > 0)
			set_bitmap(sb, PINTFS_BLOCK_BITMAP_BLOCK, pii->i_data[i], 0);
	}

	set_bitmap(sb, PINTFS_INODE_BITMAP_BLOCK, ino, 0);
	
    truncate_inode_pages_final(&inode->i_data);

    clear_inode(inode);

	mark_inode_dirty(inode);
	
}

/*
	pintfs_new_inode - Make new pintfs_inode and record in disk
*/
struct inode *pintfs_new_inode(const struct inode *dir, umode_t mode)
{
	struct super_block *sb;
	struct inode *inode;
	int new_ino;
	struct timespec64 cur_time;

	if (DEBUG)
		printk("pintfs - new inode\n");
	
	if(!dir)
		return NULL;
	sb = dir->i_sb;

	inode = new_inode(sb);
	if(!inode)
		return NULL;

	new_ino = pintfs_empty_inode(sb);

	if(new_ino == -1){
		printk("pintfs - inode table is full.\n");
		return NULL;
	}

	set_bitmap(sb, PINTFS_INODE_BITMAP_BLOCK, new_ino, 1);
	inode_init_owner(inode, dir, mode);
	cur_time = current_time(inode);

	inode->i_ino = new_ino;
	inode->i_mode = mode;
	inode->i_size = 0;
	inode->i_ctime = inode-> i_mtime = inode->i_atime = cur_time;

	// Write pintfs_inode in disk!
	pintfs_write_inode(sb, inode);
	insert_inode_hash(inode);
	return inode;
}

/*
	pintfs_get_inode - get bh data
*/
static struct pintfs_inode *pintfs_get_inode(struct super_block *sb, ino_t ino,
		struct buffer_head **p)
{
	struct buffer_head *bh;
	unsigned long block;
	unsigned long offset;

	if(DEBUG)
		printk("pintfs - pintfs_get_inode\n");

	*p = NULL;
	if ((ino != PINTFS_ROOT_INO && ino < PINTFS_GOOD_FIRST_INO) ||
			ino > PINTFS_SB(sb)->s_es->inodes_count)
		goto Einval;

	offset = (ino - 1) % PINTFS_INODES_PER_BLOCK * PINTFS_INODE_SIZE;
	block = PINTFS_FIRST_INODE_BLOCK + (ino - 1) / PINTFS_INODES_PER_BLOCK;
	
	if(!(bh = sb_bread(sb, block)))
		goto Eio;

	*p = bh;
	return (struct pintfs_inode *) (bh->b_data + offset);

Einval:
	printk("pintfs - pintfs_get_inode: bad inode number\n");
	return ERR_PTR(-EINVAL);
Eio:
	printk("pintfs - pintfs_get_inode: unable to read inode block\n");
	return ERR_PTR(-EIO);
}

/*
	pintfs_iget - fill pintfs data in inode
*/
struct inode *pintfs_iget(struct super_block *sb, unsigned long ino)
{	
	struct buffer_head *bh = NULL;
	struct pintfs_inode *raw_inode;
	struct pintfs_inode_info *pi;
	struct inode *inode;
	long ret = -EIO;
	uid_t i_uid;
	int i;

	if (DEBUG)
		printk("pintfs - pintfs_iget\n");

	inode = iget_locked(sb, ino);
	if(!inode)
		return ERR_PTR(-ENOMEM);
	if(!(inode->i_state & I_NEW)){
		return inode;
	}
	raw_inode = pintfs_get_inode(inode->i_sb, ino, &bh);
	print_pintfs_inode(raw_inode);
	if(IS_ERR(raw_inode)) {
		ret = PTR_ERR(raw_inode);
		goto bad_inode;
	}

	inode->i_ino = ino;
	inode->i_sb = sb;
	inode->i_flags = 0;	
	inode->i_mode = raw_inode->i_mode;
	inode->i_size = raw_inode->i_size;
	printk("inode->i_size = %d\n");
	inode->i_ctime = inode->i_mtime = inode->i_atime = current_time(inode);

	if(S_ISDIR(raw_inode->i_mode)){
		inode->i_op = &pintfs_dir_inode_ops;
		inode->i_fop = &pintfs_dir_ops;
	}
	else{
		inode->i_op = &pintfs_file_inode_ops;
		inode->i_fop = &pintfs_file_ops;
	}

	i_uid = raw_inode->i_uid;
	i_uid_write(inode, i_uid);
	
	pi = PINTFS_I(inode);	
	for(i=0; i<PINTFS_N_BLOCKS; i++){
		pi->i_data[i] = raw_inode->i_block[i];
	}

	if (DEBUG)
		printk("pintfs - pintfs_iget ok, inode=%p\n", inode);

	unlock_new_inode(inode);
	brelse(bh);
	return inode;

bad_inode:
	brelse(bh);
	iput(inode);
	return ERR_PTR(ret);
}


static int pintfs_setattr(struct dentry *dentry, struct iattr *attr){
	return 0;
}

/*
   INODE_OPERATIONS
*/
const struct inode_operations pintfs_file_inode_ops = {
	.setattr = pintfs_setattr,
};


