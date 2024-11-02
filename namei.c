#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/buffer_head.h>

#include "pintfs.h"

#define DEBUG 1


/*
   pintfs_create - create a new file in a directory
*/
static int pintfs_create(struct inode *dir, struct dentry* dentry, umode_t mode, bool excl)
{
	struct inode *inode;
	struct buffer_head *bh;
	struct pintfs_dir_entry *pde;
	int i, num_dirs;
	if (DEBUG)
		printk("pintfs - create\n");

	inode = pintfs_new_inode(dir, S_IFREG | mode);
	if(!inode)
		return -ENOSPC;
	
	inode->i_op = &pintfs_file_inode_ops;
	inode->i_fop = &pintfs_file_ops;
	inode->i_mode = mode;

	// Write pintfs_dir_entry in dir!
	bh = pintfs_sb_bread_dir(dir); 
	num_dirs = NUM_DIRS;
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
		return -ENOSPC;
	}

	strncpy(pde->name, dentry->d_name.name, dentry->d_name.len);
	pde->name[dentry->d_name.len] = '\0';
	pde->inode_number = inode->i_ino;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
	pintfs_write_inode(inode->i_sb, inode);

	dir->i_size += sizeof(struct pintfs_dir_entry);
	dir->i_atime = current_time(dir);
	d_instantiate(dentry, inode);
	pintfs_write_inode(dir->i_sb, dir);

	mark_inode_dirty(dir);
	mark_inode_dirty(inode);

	printk("pintfs - File created\n");
	return 0;
}
/*
   pintfs_lookup - find pintfs_dir_entry
*/
static struct dentry *pintfs_lookup(struct inode *dir, 
		struct dentry *dentry, unsigned int flags)
{

	struct buffer_head *bh;
	struct inode *inode = NULL;
	int i, num_dirs;
	struct pintfs_dir_entry *pde;

	if(DEBUG)
		printk("pintfs - lookup\n");

	bh = pintfs_sb_bread_dir(dir); 
	if(!bh)
		return ERR_PTR(-EIO);

	num_dirs = PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);
	
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

static int pintfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode;
	struct pintfs_inode_info *pii;
	struct buffer_head *bh;
	int num_dirs, i, new_blockno;
	struct pintfs_dir_entry *pde;
	if(DEBUG)
		printk("pintfs - mkdir\n");

	// Just get inode_number?
	// NO. Set block too.
	inode = pintfs_new_inode(dir, S_IFDIR | mode);
	if(!inode)
		return -ENOSPC;
	inode->i_op = &pintfs_dir_inode_ops;
	inode->i_fop = &pintfs_dir_ops;
	inode->i_mode = S_IFDIR | mode;

	new_blockno = pintfs_empty_block(dir->i_sb);
	pii = PINTFS_I(inode);
	pii->i_data[0] = new_blockno;
	set_bitmap(dir->i_sb, PINTFS_BLOCK_BITMAP_BLOCK, new_blockno, 1);

	if (!dir)
		return -1;
	
	bh = pintfs_sb_bread_dir(dir); 
	num_dirs = PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);

	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));
		if(pde->inode_number == 0)
			break;
	}

	strncpy(pde->name, dentry->d_name.name, dentry->d_name.len);
	pde->name[dentry->d_name.len] = '\0';
	pde->inode_number = inode->i_ino;
	
	pintfs_write_inode(inode->i_sb, inode);

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	dir->i_size += sizeof(struct pintfs_dir_entry);
	dir->i_atime = current_time(dir);
	d_instantiate(dentry, inode);
	pintfs_write_inode(dir->i_sb, dir);

	mark_inode_dirty(dir);
	printk("Directory created\n");
	return 0;
}

//TODO: later.. 10.13	
static int pintfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct buffer_head *bh;
	struct inode *inode;
	int num_dirs, i, k;
	struct pintfs_dir_entry *pde, *dent1, *dent2;
	if(DEBUG)
		printk("pintfs - unlink\n");

	bh = pintfs_sb_bread_dir(dir); 
	num_dirs =  PINTFS_BLOCK_SIZE / sizeof(struct pintfs_dir_entry);

	for(i=0; i<num_dirs; i++){
		pde = (struct pintfs_dir_entry *)((bh->b_data) + i*sizeof(struct pintfs_dir_entry));
		if(strlen(pde->name) == dentry->d_name.len &&
				strncmp(pde->name, dentry->d_name.name, dentry->d_name.len) == 0){
			inode = pintfs_iget(dir->i_sb, pde->inode_number);
			if(!inode)
				return -ENONET;

			dir->i_size -= sizeof(struct pintfs_dir_entry);
			for(k=i+1; k< num_dirs; k++){
				dent1 = (struct pintfs_dir_entry *) ((bh->b_data) + (k-1)*sizeof(struct pintfs_dir_entry));
				dent2 = (struct pintfs_dir_entry *) ((bh->b_data) + (k)*sizeof(struct pintfs_dir_entry));
				strcpy(dent1->name, dent2->name);
				dent1->inode_number = dent2->inode_number;
			}
			
			mark_buffer_dirty(bh);
			sync_dirty_buffer(bh);	
			pintfs_write_inode(dir->i_sb, dir);

			mark_inode_dirty(dir);
			inode_dec_link_count(inode);

			brelse(bh);
			return 0;
		}
	}
	brelse(bh);
	return -ENOENT;
}


static int pintfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);
	int err = -ENOTEMPTY;

	if (DEBUG)
		printk("pintfs - rmdir\n");

	if(pintfs_empty_dir(inode)){
		err = pintfs_unlink(dir, dentry);
		if(!err) {
			inode->i_size = 0;
			//inode_dec_link_count(inode);
		}
	}

	set_bitmap(inode->i_sb, PINTFS_INODE_BITMAP_BLOCK, inode->i_ino, 0);
	set_bitmap(inode->i_sb, PINTFS_BLOCK_BITMAP_BLOCK, PINTFS_I(inode)->i_data[0], 0);
	return err;
}

// tunrcate ???
static int pintfs_setattr(struct dentry *dentry, struct iattr *iattr)
{
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
