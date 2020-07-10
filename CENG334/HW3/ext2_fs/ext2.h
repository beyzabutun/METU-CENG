#ifndef _EXT2_H_
#define _EXT2_H_

#include "ext2_fs.h"
#include "vfs/fs.h"
#include <stdio.h>

struct file_system_type myfs;

struct file_system_type *initialize_ext2(const char *image_path);

void read_group_descriptor(struct file_system_type *fs);
unsigned int get_inode_table();
off_t get_baseoffset();

void ext2_read_inode(struct inode *i);
int ext2_statfs(struct super_block *sb, struct kstatfs *stats);

struct dentry *ext2_lookup(struct inode *i, struct dentry *dir);
int ext2_readlink(struct dentry *dir, char *buf, int len);
int ext2_readdir(struct inode *i, filldir_t callback);
int ext2_getattr(struct dentry *dir, struct kstat *stats);


loffset_t ext2_llseek(struct file *f, loffset_t o, int whence);
ssize_t ext2_read(struct file *f, char *buf, size_t len, loffset_t *o);
int ext2_open(struct inode *i, struct file *f);
int ext2_release(struct inode *i, struct file *f);

#endif
