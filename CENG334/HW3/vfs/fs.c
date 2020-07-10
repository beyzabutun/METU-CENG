#include "fs.h"
#include "ext2_fs/ext2.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int init_fs(const char *image_path) {
  current_fs = initialize_ext2(image_path);
  current_sb = current_fs->get_superblock(current_fs);
  return !(current_fs && current_sb);
}

struct file *openfile(char *path) {
  struct file *f = malloc(sizeof(struct file));
  f->f_path = malloc(strlen(path) + 1);
  strcpy(f->f_path, path);
  struct dentry *dir = pathwalk(path);
  if (!dir) {
    return NULL;
  }
  f->f_inode = dir->d_inode;
  free(dir);
  if (f->f_inode->f_op->open(f->f_inode, f)) {
    return NULL;
  }
  return f;
}

int closefile(struct file *f) {
  if (f->f_op->release(f->f_inode, f)) {
    printf("Error closing file\n");
  }
  free(f);
  f = NULL;
  return 0;
}

int readfile(struct file *f, char *buf, int size, loffset_t *offset) {
  if (*offset >= f->f_inode->i_size) {
    return 0;
  }
  if (*offset + size >= f->f_inode->i_size) {
    size = f->f_inode->i_size - *offset;
  }
  // May add llseek call
  return f->f_op->read(f, buf, size, offset);
}

struct dentry *pathwalk(char *path) {
  /* Allocates and returns a new dentry for a given path */
  unsigned int inodes_count = current_sb->s_inodes_count;
  unsigned int inode_table = get_inode_table();
  unsigned long block_size = current_sb->s_blocksize;
  struct dentry *dent;
  struct dentry *parent_dent;
  struct inode *inode = malloc(current_sb->s_inode_size);
  struct ext2_inode ext2_root_inode;
  
  //Create root directory
  inode->i_ino = 2;
  ext2_read_inode(inode);
  parent_dent = malloc(sizeof(struct dentry));
  parent_dent->d_flags = current_sb->s_root->d_flags;
  parent_dent->d_inode = inode;
  parent_dent->d_parent = NULL;
  parent_dent->d_name = malloc(strlen("/") + 1);
  strcpy(parent_dent->d_name, ".\n");
  parent_dent->d_sb = current_sb;

  if(path[0] == '.'){
    return parent_dent;
  }
  char * token = strtok(path, "/");
  while( token != NULL ) {
    dent = malloc(sizeof(struct dentry));
    dent->d_name = malloc(strlen(token) + 1);
    strcpy(dent->d_name, token);
    dent = ext2_lookup(inode, dent);
    inode = dent->d_inode;
    if(inode == NULL) return NULL;
    token = strtok(NULL, "/");
    if(token!=NULL){
      parent_dent = dent;
      free(dent);
    }
  }
  dent->d_parent = parent_dent;
  return dent;
}

int statfile(struct dentry *d_entry, struct kstat *k_stat) {
  return d_entry->d_inode->i_op->getattr(d_entry, k_stat);
}
