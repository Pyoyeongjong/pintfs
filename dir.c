#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include "pintfs.h"

//TODO:
static int 
pintfs_readdir(struct file *filp, struct dir_context *ctx)
{	
	struct inode *i;
	struct buffer_head *bh;
	int num_dirs;
	int ino;

	//...

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
