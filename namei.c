#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

#include "pintfs.h"

#define DEBUG 1
/*
   pintfs_create - create a new file in a directory
*/
static int pintfs_create(struct inode *dir, struct dentry* dentry, umode_t mode, bool excl)
{
	struct inode *inode;
	struct pintfs_inode* pi;
	int err;
	struct buffer_head *bh;
	struct pintfs_dir_entry *pde;
	int i;
	if (DEBUG)
		printk("pintfs - create\n");

	inode = pintfs_new_inode(dir, S_IRUGO|S_IWUGO|S_IFREG);
	if(!inode)
		return -ENOSPC;
	
	inode->i_op = &pintfs_file_inode_operations;
	inode->i_fop = &pintfs_file_operations;
	inode->i_mode = mode;

	// Write pintfs_dir_entry in dir!
	bh = sb_bread(dir->i_sb, PINTFS_I(dir)->i_block[0]); 
	num_dirs = PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);
	pde = (struct pintfs_dir_entry *)((bh->b_data));
	for(i=0; i<num_dirs; i++){
		if (pde->inode_number == 0){
			break;
		}
		pde++;
	}
	if(i == num_dirs){
		brelse(bh);
		iput(inode);
		return -ENOSPC:
	}

	strncpy(pde->name, dentry->d_name.name, dentry->d_name.len);
	pde->name[dentry->d_name.len] = '\0';
	pde->inode_number = inode->i_ino;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	d_instantiate(dentry, inode);
	mark_inode_dirty(dir);
	mark_inode_dirty(inode);

	printk("pintfs - File created\n");
	return 0;
}
/*
   pintfs_lookup - find pintfs_dir_entry
*/
static struct dentry *pint_lookup(struct inode *dir, 
		struct dentry *dentry, unsigned int flags)
{

	struct buffer_head *bh;
	struct inode *inode = NULL;
	int i, num_dirs;
	struct pintfs_dir_entry *pde;

	if(DEBUG)
		printk("pintfs - lookup\n");

	bh = sb_bread(dir->i_sb, PINTFS_I(dir)->i_data[0]);
	if(!bh)
		return ERR_PTR(-EIO);

	num_dirs = PINTFS_BLOC_SIZE / sizeof(struct pintfs_dir_entry);
	
	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));

		if(pde->inode_number <= 0)
			continue;

		if(strlen(pde->name) == dentry->d_name.len &&
				strncmp(pde->name, dentry->d_name.name, dentry->d_name.len) == 0){ 
			inode = pintfs_iget(dir->i_sb, pde->inode_number);

			if(!inode){
				brelse(bh);
				return ERR_PTR(-ENOENT);
			}
			
			brelse(bh);
			d_add(dentry, inode);
			return NULL;
		}
	}

	brelse(bh);
	d_add(dentry, inode);
	return NULL;
}

static int pint_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode;
	struct buffe_head *bh;
	int num_dirs, i;
	struct pintfs_dir_entry *pde;
	if(DEBUG)
		printk("pintfs - mkdir\n");

	// Just get inode_number
	inode = pintfs_new_inode(dir, S_IFDIR | mode);
	if(!inode)
		return -ENOSPC;
	inode->i_op = &pintfs_dir_inode_operations;
	inode->i_fop = &pintfs_dir_operations;
	inode->i_mode = S_IFDIR | mode;

	if (!dir)
		return -1;
	
	bh = sb_bread(dir->i_sb, PINTFS_I(dir)->i_data[0]);
	num_dirs = PINTFS_BLOCK_SIZE / sizeof(pintfs_dir_entry);

	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));
		if(pde->inode_number == 0)
			break;
	}

	strncpy(pde->name, dentry->d_name.name, dentry->d_name.len);
	pde->name[dentry->d_name.len] = '\0';
	pde->inode_number = inode->i_ino;

	mark_buffer_dirty(bh);
	sync_dirty_bhffer(bh);
	brelse(bh);

	d_instantiate(dentry, inode);

	mark_inode_dirty(dir);
	printk("Directory created\n");
	return 0;
}

static int pintfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct buffer_head *bh;
	int num_dirs, i;
	struct pintfs_dir_entry *pde;
	if(DEBUG)
		printk("pintfs - unlink\n");

	bh = sb_bread(dir->i_sb, PINTFS_I(dir)->i_data[0]);
	num_dirs =  PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);

	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));
		if(strlen(pde->name) == dentry->d_name.len &&
				strncmp(pde->name, dentry->d_name.name) == 0){
			inode = pintfs_iget(dir->i_sb, pde->inode_number);
			if(!inode)
				return -ENONET;
			remove_unused_inodes(inode->i_sb, inode->i_ino);
			//TODO: later.. 10.13	
		}
	}
}


static int pint_rmdir(struct inode *inode, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	int err = -ENOTEMPTY;

	if(pintfs_emtpy_dir(inode)){
		err = pintfs_unlink(dir, dentry);
		if(!err) {
			inode->i_size = 0;
			inode_dec_link_count(inode);
		}
	}
	return err;
	
	return 0;
}

const struct inode_operations pintfs_dir_inode_ops = {
	.create = pintfs_create,
	.lookup	= pintfs_lookup,
	.unlink = pintfs_unlink,
	.mkdir	= pintfs_mkdir,
	.rmdir	= pintfs_rmdir,
	.setattr = pintfs_setattr,
};
