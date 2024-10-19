#include <linux/kernel.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/time64.h>

#include "pintfs.h"

#define DEBUG 1

/* inode_bitmap functions */

int set_inode_bitmap(struct super_block *sb, int ino, int val)
{
	struct buffer_head *bh;
	int block_no = pintfs_get_blocknum(ino);
	struct pintfs_super_block *psb = PINTFS_SB(sb)->s_es;

	if(ino >= psb->inodes_count)
		return -1;

	bh = sb_bread(sb, ino);
	if(!bh)
		return -EINVAL;

	bh->b_data[ino] = val;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	return 0;
}

static int pintfs_empty_inode(struct super_block *sb)
{
	struct pintfs_sb_info *sbi;
	struct pintfs_super_block *psb;
	struct buffer_head *bh;
	int inode_bitmap_bno; /* inode bitmap block number */
	char inode_bitmap[PINTFS_INODE_BITMAP_SIZE];
	int result = -1, i;
	
	sbi = PINTFS_SB(sb);

	if(!sbi)
		return result;

	psb = sbi->s_es;
	inode_bitmap_bno = psb->inode_bitmap_block;

	bh = sb_bread(sb, inode_bitmap_bno);
	if(!bh)
		return result;

	memcpy(inode_bitmap, bh->b_data, PINTFS_INODE_BITMAP_SIZE);	
	for(i=PINTFS_GOOD_FIRST_INO; i<PINTFS_INODE_BITMAP_SIZE; i++){
		if(inode_bitmap[i] == 0){
			result = i;
			break;
		}
	}
	
	brelse(bh);
	return result;
}


/*
   이거 필요함?? 생각을 해봐야함
*/
int remove_unused_inodes(struct super_block *sb, int inode_no)
{
	if (inode_no == -1)
		return 0;

	struct pintfs_inode *pi;
}
/*
	pintfs_write_inode - Write pintfs_inode in block device
*/
// TODO: You must write pintfs_inode_info data in disk!!
static int pintfs_write_inode(struct super_block *sb, int inum, struct pintfs_inode* pinode)
{
	struct buffer_head *bh;

	if(DEBUG)
		printk("pintfs - write_inode in block device (inum:%d)\n", inum);

	bh = sb_bread(sb, pintfs_get_blocknum(inum));
	if(!bh)
		return -ENOSPC;
	memcpy(bh->b_data, pinode, PINTFS_INODE_SIZE);
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	if(DEBUG)
		printk("pintfs - write_inode done (inum=%d)\n", inum);
	return PINTFS_INODE_SIZE;	
}


/* 
	pintfs_empty_inode - Seek inode bitmap and return smallest available ino
*/

/*
	pintfs_new_inode - Make new pintfs_inode and record in disk
*/
static struct inode *pintfs_new_inode(const struct inode *dir, umode_t mode)
{
	struct pintfs_inode pinode;
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

	set_inode_bitmap(sb, new_ino, 1);
	inode_init_owner(inode, dir, mode);
	cur_time = current_time(inode);

	pinode.i_mode = mode;
	pinode.i_uid = i_uid_read(inode);
	pinode.i_size = 0;
	pinode.i_time = cur_time.tv_sec;

	// Write pintfs_inode in disk!
	pintfs_write_inode(sb, new_ino, &pinode);

	inode->i_ino = new_ino;
	inode->i_ctime = inode-> i_mtime = inode->i_atime = cur_time;
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
	gid_t i_gid;
	int i;

	inode = new_inode(sb);
	if(!inode)
		return ERR_PTR(-ENOMEM);
	if(!(inode->i_state & I_NEW))
		return inode;

	raw_inode = pintfs_get_inode(inode->i_sb, ino, &bh);
	if(IS_ERR(raw_inode)) {
		ret = PTR_ERR(raw_inode);
		goto bad_inode;
	}

	inode->i_ino = ino;
	inode->i_sb = sb;
	if(raw_inode->i_mode){
		inode->i_op = &pintfs_dir_inode_ops;
		inode->i_fop = &pintfs_dir_ops;
	}
	else{
		inode->i_op = &pintfs_file_inode_ops;
		inode->i_fop = &pintfs_file_ops;
	}

	inode->i_mode = raw_inode->i_mode;
	i_uid = raw_inode->i_uid;
	i_gid = raw_inode->i_uid;
	i_uid_write(inode, i_uid);
	i_gid_write(inode, i_gid);
	
	//set_nlink() - it is for link
	inode->i_size = raw_inode->i_size;
	inode->i_atime.tv_sec = raw_inode->i_time;
	inode->i_ctime.tv_sec = raw_inode->i_time;
	inode->i_mtime.tv_sec = raw_inode->i_time;

	inode->i_flags = 0;

	pi = PINTFS_I(inode);	
	for(i=0; i<PINTFS_N_BLOCKS; i++){
		pi->i_data[i] = raw_inode->i_block[i];
	}

	brelse(bh);
	return inode;

bad_inode:
	brelse(bh);
	iput(inode);
	return ERR_PTR(ret);
}

/*
	pintfs_setattr
*/
static int pintfs_setattr(struct dentry *dentry, struct iattr *iattr)
{
	return 0;
}
/*
   INODE_OPERATIONS
*/
const struct inode_operations pintfs_file_inode_ops = {
	.setattr	=	pintfs_setattr,
};


