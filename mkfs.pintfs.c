#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // pwrite
#include <fcntl.h> // open

#define PINTFS_MAGIC_NUMBER 0xDEADBEEE
#define PINTFS_BLOCK_SIZE (1<<12)

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


void init_super_block(int fd){
	struct pintfs_super_block sb;

	sb.magic = PINTFS_MAGIC_NUMBER;
	sb.block_size = PINTFS_BLOCK_SIZE;
	sb.inodes_count = 128;
	sb.blocks_count = 64;
	sb.free_blocks = sb.blocks_count - 4;
	sb.free_inodes = sb.inodes_count - 1;
	sb.inode_bitmap_block = 1;
	sb.block_bitmap_block = 2;
	sb.first_inode_block = 3;
	sb.first_data_block = 5;

	// Disk가 FD 형식으로 전달된다!
	if (pwrite(fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
		perror("Failed to wrtie pintfs_superblock");
		close(fd);
		exit(1);
	}	
}

void init_bitmaps(int fd){
	unsigned char bitmap_block[PINTFS_BLOCK_SIZE];
	memset(bitmap_block, 0, PINTFS_BLOCK_SIZE);

	if (pwrite(fd, bitmap_block, PINTFS_BLOCK_SIZE, PINTFS_BLOCK_SIZE * 1) != PINTFS_BLOCK_SIZE) {
		perror("Failed to wrtie inode_bitmap");
		close(fd);
		exit(1);
	}	
	
	if (pwrite(fd, bitmap_block, PINTFS_BLOCK_SIZE, PINTFS_BLOCK_SIZE * 2) != PINTFS_BLOCK_SIZE) {
		perror("Failed to wrtie block_bitmap");
		close(fd);
		exit(1);
	}
}



int main(int argc, char *argv[]) {
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <device>\n", argv[0]);
		exit(1);
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0){
		perror("Failed to Open Device");
		exit(1);
	}

	init_super_block(fd);
	init_bitmaps(fd);
	//TODO: Should I init Root inode?
	printf("Pintfs init successed on %s\n",argv[1]);
	close(fd);

	return 0;
}
