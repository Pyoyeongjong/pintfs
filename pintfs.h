#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>

#include "pintfs_common.h"

/* super.c */
extern const struct super_operations pintfs_super_ops;
/* file.c */
extern const struct file_operations pintfs_file_ops;
/* inode.c */
extern const struct inode_operations pintfs_file_inode_ops;
struct inode *pintfs_iget(struct super_block *sb, unsigned long ino);
/* dir_c */
extern const struct file_operations pintfs_dir_ops;
/* namei.c */
extern const struct inode_operations pintfs_dir_inode_ops;

static inline struct pintfs_sb_info *PINTFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline int pintfs_get_blocknum(int inum)
{
	int inodes_per_block = PINTFS_BLOCK_SIZE / PINTFS_INODE_SIZE;
	return PINTFS_FIRST_INODE_BLOCK + inum / inodes_per_block; 
}

static inline struct pintfs_inode_info *PINTFS_I(struct inode* inode)
{
	return container_of(inode, struct pintfs_inode_info, vfs_inode);
}

