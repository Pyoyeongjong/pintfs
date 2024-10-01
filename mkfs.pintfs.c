#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // pwrite
#include <fcntl.h> // open
#include <time.h>

#include "pintfs_common.h"

/*
   init_super_block - Write superblock metadata in 0st block
*/
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

	// Disk is handled like file!
	if (pwrite(fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
		perror("Failed to wrtie pintfs_superblock");
		close(fd);
		exit(1);
	}	
}

/*
   init_bitmaps - Write bitmap datas in 1st, 2nd block
*/
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

#define ISDIR		1
#define ISREG		0
/*
   init_root_inode_info - Write root inode in 3rd block
*/
void init_root_inode_info(int fd)
{
	struct pintfs_inode root_inode;
	
	root_inode.i_mode = ISDIR;
	root_inode.i_uid = 1000;
	root_inode.i_size = 0;
	root_inode.i_time = time(NULL);
	for(int i=0; i<PINTFS_N_BLOCKS; i++){
		if(i == 0)
			root_inode.i_block[i] = PINTFS_FIRST_DATA_BLOCK; //Not 5th block address, Just 5!
		else
			root_inode.i_block[i] = 0;
	}
	root_inode.i_blocks = 0; 

	if (pwrite(fd, &root_inode, sizeof(struct pintfs_inode), PINTFS_BLOCK_SIZE * 3) 
				!= sizeof(struct pintfs_inode))
	{
		perror("Failed to wrtie root_inode");
		close(fd);
		exit(1);
	}
}

/*
   write_root_dir_entry - Write root dir_entry int 5th block
*/
void write_root_dir_entry(int fd)
{
	struct pintfs_dir_entry dir_entries[2];

	strcpy(dir_entries[0].name, ".");
	dir_entries[0].size = 0;
	dir_entries[0].inode_number = PINTFS_ROOT_INO;

	strcpy(dir_entries[1].name, "..");
	dir_entries[1].size = 0;
	dir_entries[1].inode_number = PINTFS_ROOT_INO;

	if(pwrite(fd, dir_entries, sizeof(dir_entries), PINTFS_BLOCK_SIZE * PINTFS_FIRST_DATA_BLOCK)
			!= sizeof(dir_entries))
	{
		perror("Failed to write root_dir_entry");
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
	init_root_inode_info(fd);
	write_root_dir_entry(fd);

	printf("Pintfs init successed on %s\n",argv[1]);
	close(fd);

	return 0;
}
