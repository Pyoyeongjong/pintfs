#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include "pintfs_common.h"

#define NUM_DIRS (PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry))

/*
   pintfs_inode_info
*/
struct pintfs_inode_info {
	unsigned int	i_data[15];
	struct inode	vfs_inode;
};

/*
   pintfs_sb_info - Pintfs Superblock Info
*/
struct pintfs_sb_info {
	struct pintfs_super_block *s_es; /* pintfs_super_block */
	int s_first_ino; /* First inode (2) */
	int s_inode_size;		/* Inode byte 크기 (64bytes)*/
};


/* balloc.c */
int pintfs_empty_block(struct super_block *sb);
/* super.c */
extern const struct super_operations pintfs_super_ops;
int set_bitmap(struct super_block *sb, int bno, int no, int val);
/* file.c */
extern const struct file_operations pintfs_file_ops;
/* inode.c */
extern const struct inode_operations pintfs_file_inode_ops;
int pintfs_write_inode(struct super_block *sb, struct inode* inode);
int pintfs_empty_inode(struct super_block *sb);
void pintfs_evict_inode(struct inode *inode);
struct inode *pintfs_iget(struct super_block *sb, unsigned long ino);
struct inode *pintfs_new_inode(const struct inode *dir, umode_t mode);
/* dir_c */
extern const struct file_operations pintfs_dir_ops;
int pintfs_empty_dir(struct inode *inode);
/* namei.c */
extern const struct inode_operations pintfs_dir_inode_ops;

static inline struct pintfs_sb_info *PINTFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline int pintfs_get_blocknum(int inum)
{
	return PINTFS_FIRST_INODE_BLOCK + (inum - 1) / PINTFS_INODES_PER_BLOCK; 
}

static inline struct pintfs_inode_info *PINTFS_I(struct inode* inode)
{
	return container_of(inode, struct pintfs_inode_info, vfs_inode);
}

static inline struct buffer_head *pintfs_sb_bread_dir(struct inode* inode)
{	
	printk("pintfs_sb_bread_dir - i_data[0]=%d\n",PINTFS_I(inode)->i_data[0]);
	return sb_bread(inode->i_sb, PINTFS_I(inode)->i_data[0]);
}

static inline struct buffer_head *pintfs_sb_bread_file(struct inode* inode, int block_index)
{
	int block_no = PINTFS_I(inode)->i_data[block_index];
	printk("pintfs_sb_bread_file - block_no = %d\n", block_no);
	if(block_no <= 0)
		return NULL;
	else
		return sb_bread(inode->i_sb, block_no);	
}

static inline void print_pintfs_inode(struct pintfs_inode *pi){
	printk("pi=%p, i_mode = %o, i_uid=%d, i_size= %ld, i_time=%lld\n"
			,pi, pi->i_mode, pi->i_uid, pi->i_size, pi->i_time);
}

