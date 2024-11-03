#define PINTFS_MAGIC_NUMBER 0xDEADBEEF
#define PINTFS_BLOCK_SIZE (1 << 12) /* 4KB */
#define PINTFS_N_BLOCKS		8

#define PINTFS_MAX_FILE_SIZE (PINTFS_BLOCK_SIZE) * PINTFS_N_BLOCKS;
#define PINTFS_INODE_BITMAP_SIZE	128
#define PINTFS_BLOCK_BITMAP_SIZE	64
#define PINTFS_INODES_PER_BLOCK		(PINTFS_BLOCK_SIZE / PINTFS_INODE_SIZE) 

#define PINTFS_SUPER_BLOCK			0
#define PINTFS_INODE_BITMAP_BLOCK	1
#define PINTFS_BLOCK_BITMAP_BLOCK	2
#define PINTFS_FIRST_INODE_BLOCK	3
#define PINTFS_FIRST_DATA_BLOCK		5

#define PINTFS_BAD_INO		0
#define PINTFS_ROOT_INO		1
#define PINTFS_GOOD_FIRST_INO 2

#define MAX_NAME_SIZE 15

/* 
	pintfs_super_block - Superblock Metadata (It is on 0 block)
*/
struct pintfs_super_block {
	unsigned int	magic;			/* MAGIC NUMBER */
	unsigned int	block_size;		/* 블록 크기 */
	unsigned int	inodes_count;		/* 총 Inode 개수 */
	unsigned int	blocks_count;		/* 총 block 개수 */
	unsigned int	blocksize_bits;	/* block size 비트로(12) */
	unsigned int	free_blocks;	/* 사용가능 blocks */
	unsigned int	free_inodes;	/* 사용가능 inodes */
	int				inode_bitmap_block; /* inode bitmap이 저장된 block 위치 */
	int				block_bitmap_block; /* block bitmap이 저장된 block 위치 */
	int				first_inode_block; /* pintfs_inode가 저장된 block 위치 */
	unsigned int	first_data_block;	/* 5 */
};
/*
   pintfs_inode
*/
struct pintfs_inode {
	int i_mode;		/* File mode */
	int i_uid;		/* Low 16 bits of Owner Uid */
	ssize_t i_size;		/* Size in bytes */
	long long i_time;		/* Access, Create, Modificate, or Deletion Time */
	unsigned int i_block[PINTFS_N_BLOCKS]; /* Direct 0~6, Indirect 7 */
	unsigned int i_blocks; /* How many blocks this inode uses */
};
#define PINTFS_INODE_SIZE sizeof(struct pintfs_inode)
/*
   pintfs_dir_entry - just dir_entry on disk
*/
struct pintfs_dir_entry{
	char name[MAX_NAME_SIZE];
	int inode_number;
};

