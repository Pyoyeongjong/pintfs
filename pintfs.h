#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h> // super_block

#define PINTFS_MAGIC_NUMBER 0xDEADBEEF
#define PINTFS_BLOCK_SIZE (1 << 12) /* 4KB */
#define PINTFS_N_BLOCKS		11
#define PINTFS_MAX_FILE_SIZE (PINTFS_BLOCK_SIZE) * PINTFS_N_BLOCKS;
#define PITNFS_INODE_SIZE 64
#define PINTFS_INODE_BITMAP_SIZE	128
#define PINTFS_BLOCK_BITMAP_SIZE	64
typedef unsigned short umode_t;
/* super.c */
extern const struct super_operations pintfs_super_ops;
/* file.c */
extern const struct file_operations	pintfs_file_ops;
/* inode.c */
extern const struct inode_operations pintfs_file_inode_ops;
/* dir.c */
extern const struct file_operations pintfs_dir_ops;
/* namei.c */
extern const struct inode_operations pintfs_dir_inode_ops;
struct pintfs_super_block {
	unsigned int	magic;			/* MAGIC NUMBER */
	unsigned int	block_size;		/* 블록 크기 */
	unsigned int	inodes_count;		/* 총 Inode 개수 */
	unsigned int	blocks_count;		/* 총 block 개수 */
	unsigned int	blocksize_bits;	/* block size 비트로(12) */
	unsigned int	free_blocks;	/* 사용가능 blocks */
	unsigned int	free_inodes;	/* 사용가능 inodes */
	int				inode_bitmap_block;
	int				block_bitmap_block;
	int				first_inode_block;
	unsigned int	first_data_block;	/* 5 */
};

struct pintfs_sb_info {
	struct pintfs_super_block *s_es; /* pintfs_super_block */
	int s_first_ino; /* First inode (2) */
	int s_inode_size;		/* Inode byte 크기 (64bytes)*/
};

static inline struct pintfs_sb_info *PINTFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

#define PINTFS_BAD_INO		0
#define PINTFS_ROOT_INO		1
#define PINTFS_GOOD_FIRST_INO 2
#define PINTFS_FIRST_INO(s)		(PINTFS_SB(s)->s_first_ino)
#define PINTFS_INODE_SIZE(s)	(PINTFS_SB(s)->s_inode_size)
#define PINTFS_BLOCK_SIZE_BITS(s)	((s)->s_block_size_bits) 


struct pintfs_inode {
	int i_mode;		/* File mode */
	uid_t i_uid;		/* Low 16 bits of Owner Uid */
	ssize_t i_size;		/* Size in bytes */
	int i_time;		/* Access, Create, Modificate, or Deletion Time */
	unsigned int i_block[PINTFS_N_BLOCKS]; /* Direct Block List */
	unsigned int i_blocks; /* Indirect Block List - UNUSED */
};

