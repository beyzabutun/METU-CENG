#include "ext2.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <math.h>
#define BASE_OFFSET 1024 /* location of the superblock in the first group */

struct super_operations s_op;
struct inode_operations i_op;
struct file_operations f_op;

char fs_name[] = "ext2";

/*Global Variables*/
struct ext2_super_block superBlock;
struct super_block sup_block;
unsigned long block_size;
unsigned short inode_size;
unsigned int inodes_count;
unsigned int blocks_count; 
struct ext2_group_desc group1_desc;
unsigned int block_bitmap;        /* Blocks bitmap block */
unsigned int inode_bitmap;        /* Inodes bitmap block */
unsigned int inode_table;         /* Inodes table block */
struct inode *root_inode;
//Read Group Descriptor
void read_group_descriptor(struct file_system_type *fs){
  unsigned long first_value;
  first_value = BASE_OFFSET + sizeof(superBlock);
  off_t offset = ( (first_value / block_size) + ((first_value % block_size) != 0) )*block_size; 
  //off_t offset = block_size*(ceil((BASE_OFFSET+sizeof(superBlock))/block_size));
  lseek(myfs.file_descriptor, offset, SEEK_SET);
  read(myfs.file_descriptor, &group1_desc, sizeof(group1_desc));
  block_bitmap = group1_desc.bg_block_bitmap;
  inode_bitmap = group1_desc.bg_inode_bitmap;
  inode_table = group1_desc.bg_inode_table;
}
unsigned int get_inode_table(){
  return inode_table;
}
off_t get_baseoffset(){
  return BASE_OFFSET;
}

/*Implement functions in s_op, i_op, f_op here */

//super_block (s_op) functions
void ext2_read_inode(struct inode *i){
  /*This function assumes that only the inode number field
  (i_ino) of the passed in inode i is valid and the fucntion
  reads and populates the remaining fields of i.*/
  struct ext2_inode inode_ext2;
  lseek(myfs.file_descriptor,
    BASE_OFFSET + block_size*(inode_table-1) + ((i->i_ino-1)*inode_size), SEEK_SET);
  read(myfs.file_descriptor, &inode_ext2, sizeof(inode_ext2));
  i->i_mode = inode_ext2.i_mode;
  i->i_nlink = inode_ext2.i_links_count;
  i->i_uid = inode_ext2.i_uid;
  i->i_gid = inode_ext2.i_gid;
  i->i_size = inode_ext2.i_size;
  i->i_atime = inode_ext2.i_atime;
  i->i_mtime = inode_ext2.i_mtime;
  i->i_ctime = inode_ext2.i_ctime;
  i->i_blocks = inode_ext2.i_blocks;
  //i->i_state 
  i->i_flags = inode_ext2.i_flags;
  for(int j=0; j<15; j++){
    i->i_block[j] = inode_ext2.i_block[j];
  }
  i->f_op = &f_op;
  i->i_op = &i_op;
  i->i_sb = &sup_block;


}
int ext2_statfs(struct super_block *sb, struct kstatfs *stats){
  /*This function fills in the fields of kstatfs struct stats 
  with the information from the superblock sb. Returns zero for
  successful operations.*/
  stats->name = fs_name;
  stats->f_magic = sb->s_magic;
  stats->f_bsize = sb->s_blocksize;
  stats->f_blocks = sb->s_blocks_count; 
  stats->f_bfree = sb->s_free_blocks_count;
  stats->f_inodes = sb->s_inodes_count;
  stats->f_finodes = sb->s_free_inodes_count;
  stats->f_inode_size = sb->s_inode_size;
  stats->f_minor_rev_level = sb->s_minor_rev_level;
  stats->f_rev_level = sb->s_rev_level; 
  stats->f_namelen = sizeof(fs_name);
  return 0;
}


//i_node (i_op) functions
struct dentry *ext2_lookup(struct inode *i, struct dentry *dir){
  /* This function assumes that only the name field (d_name) 
  of the directory entry dir is valid and searches for it in the
  directory pointed by inode i. If a matching directory entry is
  found, it fills the rest of the directory entry dir. 
  It returns the pointer to the filled directory entry.
  */
  int n_dir = 0;
  int tot_len;
  unsigned int  *ind_block = malloc(block_size), *ind_block2 = malloc(block_size), *ind_block3 = malloc(block_size);
  void *readed_block = malloc(block_size);
  unsigned int block_n;
  struct ext2_dir_entry *dir_entry;
  struct ext2_inode ext2_inode;
  struct inode *dir_inode = malloc(inode_size);

  for(int j=0; j<15; j++){
    if(j<12){
      block_n = i->i_block[j];
      if(block_n==0)
        break;
      tot_len=0;
      off_t offset = BASE_OFFSET + (block_n-1)*block_size;
      lseek(myfs.file_descriptor, offset, SEEK_SET);
      read(myfs.file_descriptor, readed_block, block_size);
      dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
      while(tot_len < block_size){
        char *name_pointer = &dir_entry->name[0];
        if(strcmp(name_pointer, dir->d_name)==0){
          dir->d_sb=&sup_block;
          dir_inode->i_ino = dir_entry->inode;
          ext2_read_inode(dir_inode);
          dir->d_inode = dir_inode;
          return dir;
        }
        tot_len += dir_entry->rec_len;
        if(tot_len==block_size) break;
        else dir_entry = (void *)dir_entry + dir_entry->rec_len;   
      }
    }
    else if(j==12){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        if(block_n==0)
            continue;
        tot_len=0;
        off_t offset = BASE_OFFSET + (block_n-1)*block_size;
        lseek(myfs.file_descriptor, offset, SEEK_SET);
        read(myfs.file_descriptor, readed_block, block_size);
        dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
        while(tot_len < block_size){
          char *name_pointer = &dir_entry->name[0];
          if(strcmp(name_pointer, dir->d_name)==0){
            dir->d_sb=&sup_block;
            dir_inode->i_ino = dir_entry->inode;
            ext2_read_inode(dir_inode);
            dir->d_inode = dir_inode;
            free(ind_block);
            return dir;
          }
          tot_len += dir_entry->rec_len;
          if(tot_len==block_size) break;
          else dir_entry = (void *)dir_entry + dir_entry->rec_len;   
        }
      }
    }
    else if(j==13){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=0; l < block_size/sizeof(unsigned int); l++){
          block_n = ind_block[l];
          if(block_n==0)
              continue;
          tot_len=0;
          off_t offset = BASE_OFFSET + (block_n-1)*block_size;
          lseek(myfs.file_descriptor, offset, SEEK_SET);
          read(myfs.file_descriptor, readed_block, block_size);
          dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
          while(tot_len < block_size){
            char *name_pointer = &dir_entry->name[0];
            if(strcmp(name_pointer, dir->d_name)==0){
              dir->d_sb=&sup_block;
              dir_inode->i_ino = dir_entry->inode;
              ext2_read_inode(dir_inode);
              dir->d_inode = dir_inode;
              free(ind_block); free(ind_block2);
              return dir;
            }
            tot_len += dir_entry->rec_len;
            if(tot_len==block_size) break;
            else dir_entry = (void *)dir_entry + dir_entry->rec_len;   
          }
        }
      }
    }
    else if(j==14){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=0; l < block_size/sizeof(unsigned int); l++){
          block_n = ind_block[l];
          lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
          read(myfs.file_descriptor, ind_block3, block_size);
          for(int m=0; m < block_size/sizeof(unsigned); m++){
            block_n = ind_block[m];
            if(block_n==0)
                continue;
            tot_len=0;
            off_t offset = BASE_OFFSET + (block_n-1)*block_size;
            lseek(myfs.file_descriptor, offset, SEEK_SET);
            read(myfs.file_descriptor, readed_block, block_size);
            dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
            while(tot_len < block_size){
              char *name_pointer = &dir_entry->name[0];
              if(strcmp(name_pointer, dir->d_name)==0){
                dir->d_sb=&sup_block;
                dir_inode->i_ino = dir_entry->inode;
                ext2_read_inode(dir_inode);
                dir->d_inode = dir_inode;
                free(ind_block); free(ind_block2); free(ind_block3);
                return dir;
              }
              tot_len += dir_entry->rec_len;
              if(tot_len==block_size) break;
              else dir_entry = (void *)dir_entry + dir_entry->rec_len;   
            }
          }
        }
      }
    }

  }
  free(readed_block);
  free(ind_block); free(ind_block2); free(ind_block3);
  dir->d_inode = NULL;
  return dir;
}
int ext2_readlink(struct dentry *dir, char *buf, int len){
  /*This function reads the contents of the link in dir into 
  the buffer buf provided by the user. The read operation reads
  len bytes and returns the number of bytes read.
  */
 int buf_index=0;
 for(int i=0; i<15; i++){
   if(dir->d_inode->i_block[i]==0){
     break;
   } 
   memcpy(buf + buf_index, &(dir->d_inode->i_block[i]), sizeof(dir->d_inode->i_block[i]));
   buf_index += sizeof(dir->d_inode->i_block[i]);
 }
  return buf_index;
}
int ext2_readdir(struct inode *i, filldir_t callback){
  /*This function calls the callback for every directory
   entry in inode i. It returns the total number of entries
   in the directory.
  */
  int n_dir = 0;
  int tot_len;
  unsigned int *ind_block = malloc(block_size), *ind_block2 = malloc(block_size), *ind_block3 = malloc(block_size);
  void *readed_block = malloc(block_size);
  unsigned int block_n;
  struct ext2_dir_entry *dir_entry;
  for(int j=0; j<15; j++){
    if(j<12){
      block_n = i->i_block[j];
      if(block_n==0)
        break;
      tot_len=0;
      off_t offset = BASE_OFFSET + (block_n-1)*block_size;
      lseek(myfs.file_descriptor, offset, SEEK_SET);
      read(myfs.file_descriptor, readed_block, block_size);
      dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
      while(tot_len < block_size){
        if(EXT2_FT_UNKNOWN < dir_entry->file_type  && dir_entry->file_type < EXT2_FT_MAX ){
          n_dir++;
          callback(dir_entry->name, dir_entry->name_len, dir_entry->inode);
        }
        tot_len += dir_entry->rec_len;
        if(tot_len==block_size) break;
        else dir_entry = (void *)dir_entry + dir_entry->rec_len;
      }
    }
    else if(j==12){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        if(block_n==0)
            break;
        tot_len=0;
        off_t offset = BASE_OFFSET + (block_n-1)*block_size;
        lseek(myfs.file_descriptor, offset, SEEK_SET);
        read(myfs.file_descriptor, readed_block, block_size);
        dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
        while(tot_len < block_size){
          if(EXT2_FT_UNKNOWN < dir_entry->file_type  && dir_entry->file_type < EXT2_FT_MAX ){
            n_dir++;
            callback(dir_entry->name, dir_entry->name_len, dir_entry->inode);
          }
          tot_len += dir_entry->rec_len;
          if(tot_len==block_size) break;
          else dir_entry = (void *)dir_entry + dir_entry->rec_len;
        }
      }
    }
    else if(j==13){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=0; l < block_size/sizeof(unsigned int); l++){
          block_n = ind_block[l];
          if(block_n==0)
              break;
          tot_len=0;
          off_t offset = BASE_OFFSET + (block_n-1)*block_size;
          lseek(myfs.file_descriptor, offset, SEEK_SET);
          read(myfs.file_descriptor, readed_block, block_size);
          dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
          while(tot_len < block_size){
            if(EXT2_FT_UNKNOWN < dir_entry->file_type  && dir_entry->file_type < EXT2_FT_MAX ){
              n_dir++;
              callback(dir_entry->name, dir_entry->name_len, dir_entry->inode);
            }
            tot_len += dir_entry->rec_len;
            if(tot_len==block_size) break;
            else dir_entry = (void *)dir_entry + dir_entry->rec_len;
          }
        }
      }
    }
    else if(j==14){
      block_n = i->i_block[j];
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        block_n = ind_block[k];
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=0; l < block_size/sizeof(unsigned int); l++){
          block_n = ind_block[l];
          lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
          read(myfs.file_descriptor, ind_block3, block_size);
          for(int m=0; m < block_size/sizeof(unsigned); m++){
            block_n = ind_block[m];
            if(block_n==0)
                break;
            tot_len=0;
            off_t offset = BASE_OFFSET + (block_n-1)*block_size;
            lseek(myfs.file_descriptor, offset, SEEK_SET);
            read(myfs.file_descriptor, readed_block, block_size);
            dir_entry = (struct ext2_dir_entry *) readed_block;  //fist entry
            while(tot_len < block_size){
              if(EXT2_FT_UNKNOWN < dir_entry->file_type  && dir_entry->file_type < EXT2_FT_MAX ){
                n_dir++;
                callback(dir_entry->name, dir_entry->name_len, dir_entry->inode);
              }
              tot_len += dir_entry->rec_len;
              if(tot_len==block_size) break;
              else dir_entry = (void *)dir_entry + dir_entry->rec_len;
            }
          }
        }
      }
    }
  }
  free(readed_block);
  free(ind_block); free(ind_block2); free(ind_block3);
  return n_dir;
}
int ext2_getattr(struct dentry *dir, struct kstat *stats){
  /* This function fills in the fields of kstat structure,
   stats with the information from the object pointed by the
   directory entry dir. It returns zero for successful operation.
  */
  stats->ino = dir->d_inode->i_ino;
  stats->mode = dir->d_inode->i_mode;
  stats->nlink = dir->d_inode->i_nlink;
  stats->uid = dir->d_inode->i_uid;
  stats->gid = dir->d_inode->i_gid;
  stats->size = dir->d_inode->i_size;
  stats->atime = dir->d_inode->i_atime;
  stats->mtime = dir->d_inode->i_mtime;
  stats->ctime = dir->d_inode->i_ctime;
  stats->blksize = dir->d_sb->s_blocksize;
  stats->blocks = dir->d_sb->s_blocks_count;
  return 0;
}

//file operations (f_op)
loffset_t ext2_llseek(struct file *f, loffset_t o, int whence){
  /* This function repositions the offset of the file f to o bytes
   relative to the beginning of the file, the current file offset,
  or the end of the file, depending on whether whence is SEEK_SET,
   SEEK_CUR, or SEEK_END, respectively. 
   It returns the resulting file position in the argument result.
  */
  if(whence==SEEK_SET){
    f->f_pos = o;
  }
  else if(whence==SEEK_CUR){
    f->f_pos = f->f_pos + o;
  }
  else if(whence==SEEK_END){
    f->f_pos = f->f_inode->i_size + o;
  }
  return f->f_pos;

}
ssize_t ext2_read(struct file *f, char *buf, size_t len, loffset_t *o){
  /* This function reads the contents of file f into the buffer buf
   provided by the user. The read operation starts from the o byte
   of the file and reads len bytes. It returns the number of bytes read.
  */
  unsigned int block_index;
  char *buff_temp;
  size_t size =len;
  size_t buffer_ind = 0;
  loffset_t offset=0;
  unsigned int *ind_block = malloc(block_size), *ind_block2 = malloc(block_size), *ind_block3 = malloc(block_size);
  if(o == NULL){
    block_index = f->f_pos/block_size;
    offset = f->f_pos%block_size;
  }
  else{
    block_index = *o/block_size;
    offset = *o%block_size;
  }
  while(block_index < 268+65536+16777216){  
    if(size==0) break;
    if(block_index < 12){
      unsigned int block_n = f->f_inode->i_block[block_index];
      if(block_n==0) continue;
      off_t read_offset = BASE_OFFSET + (block_n-1)*block_size + offset;
      if(size <= (block_size - offset)){
        buff_temp = malloc(size);
        lseek(myfs.file_descriptor, read_offset, SEEK_SET);
        read(myfs.file_descriptor, buff_temp, size);
        memcpy(buf+buffer_ind, buff_temp, size);
        buffer_ind += size;
        size = 0;
      }
      else{
        buff_temp = malloc(size);
        lseek(myfs.file_descriptor, read_offset, SEEK_SET);
        read(myfs.file_descriptor, buff_temp, block_size - offset);
        memcpy(buf+buffer_ind, buff_temp, block_size - offset);
        buffer_ind += block_size - offset;
        size = size - (block_size - offset);
        offset=0; 
      }
      free(buff_temp);
      block_index++;
    }
    else if(block_index >= 12 && block_index < 268){
      if(size==0) break;
      unsigned int block_n = f->f_inode->i_block[12];
      if(block_n==0) continue;
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=block_index-12; k < block_size/sizeof(unsigned int); k++){
        if(size==0) break;
        block_n = ind_block[k];
        if(block_n==0){
          continue;
        }
        off_t read_offset = BASE_OFFSET + (block_n-1)*block_size + offset;
        if(size <= (block_size - offset)){
          buff_temp = malloc(size);
          lseek(myfs.file_descriptor, read_offset, SEEK_SET);
          read(myfs.file_descriptor, buff_temp, size);
          memcpy(buf+buffer_ind, buff_temp, size);
          buffer_ind += size;
          size = 0;
        }
        else{
          buff_temp = malloc(size);
          lseek(myfs.file_descriptor, read_offset, SEEK_SET);
          read(myfs.file_descriptor, buff_temp, block_size - offset);
          memcpy(buf+buffer_ind, buff_temp, block_size - offset);
          buffer_ind += block_size - offset;
          size = size - (block_size - offset);
          offset=0; 
        }
        free(buff_temp);
        block_index++;
      }
    }
    else if(block_index >=268 && block_index < 268+65536){
      if(size==0) break;
      unsigned int block_n = f->f_inode->i_block[13];
      if(block_n==0) continue;
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        if(size==0) break;
        block_n = ind_block[k];
        if(block_n==0)
          continue;
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=block_index-k*256-268; l < block_size/sizeof(unsigned int); l++){
          if(size==0) break;
          block_n = ind_block2[l];
          if(block_n==0)
            continue;
          off_t read_offset = BASE_OFFSET + (block_n-1)*block_size + offset;
          if(size <= (block_size - offset)){
            buff_temp = malloc(size);
            lseek(myfs.file_descriptor, read_offset, SEEK_SET);
            read(myfs.file_descriptor, buff_temp, size);
            memcpy(buf+buffer_ind, buff_temp, size);
            buffer_ind += size;
            size = 0;
          }
          else{
            buff_temp = malloc(size);
            lseek(myfs.file_descriptor, read_offset, SEEK_SET);
            read(myfs.file_descriptor, buff_temp, block_size - offset);
            memcpy(buf+buffer_ind, buff_temp, block_size - offset);
            buffer_ind += block_size - offset;
            size = size - (block_size - offset);
            offset=0; 
          }
          free(buff_temp);
          block_index++;
          }
      }
    }
    else
    {
      if(size==0) break;
      unsigned int block_n = f->f_inode->i_block[13];
      if(block_n==0) continue;
      lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
      read(myfs.file_descriptor, ind_block, block_size);
      for(int k=0; k < block_size/sizeof(unsigned int); k++){
        if(size==0) break;
        block_n = ind_block[k];
        if(block_n==0)
          continue;
        lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
        read(myfs.file_descriptor, ind_block2, block_size);
        for(int l=0; l < block_size/sizeof(unsigned int); l++){
          if(size==0) break;
          block_n = ind_block2[l];
          if(block_n==0)
            continue;
          lseek(myfs.file_descriptor, BASE_OFFSET + block_size*(block_n-1), SEEK_SET);
          read(myfs.file_descriptor, ind_block3, block_size);
          for(int m=block_index-k*256*256-l*256-268-65536; m < block_size/sizeof(unsigned int); m++){
            if(size==0) break;
            block_n = ind_block3[m];
            if(block_n==0)
              continue;
            off_t read_offset = BASE_OFFSET + (block_n-1)*block_size + offset;
            if(size <= (block_size - offset)){
              buff_temp = malloc(size);
              lseek(myfs.file_descriptor, read_offset, SEEK_SET);
              read(myfs.file_descriptor, buff_temp, size);
              memcpy(buf+buffer_ind, buff_temp, size);
              buffer_ind += size;
              size = 0;
            }
            else{
              buff_temp = malloc(size);
              lseek(myfs.file_descriptor, read_offset, SEEK_SET);
              read(myfs.file_descriptor, buff_temp, block_size - offset);
              memcpy(buf+buffer_ind, buff_temp, block_size - offset);
              buffer_ind += block_size - offset;
              size = size - (block_size - offset);
              offset=0; 
            }
            free(buff_temp);
            block_index++;
          }
        }
      }

    }
  
  }
  free(ind_block); free(ind_block2); free(ind_block3);
  f->f_pos += len-size;
  return len-size;
}
int ext2_open(struct inode *i, struct file *f){
  /* This function reads the file pointed by inode i and fills the
   file struct f. It returns zero for successful operation.
  */
  f->f_op = i->f_op;
  f->f_flags = i->i_flags;
  f->f_mode = i->i_mode;
  f->f_pos = 0;
  f->f_inode->i_ino = i->i_ino;
  f->f_inode->i_mode = i->i_mode;
  f->f_inode->i_nlink = i->i_nlink;
  f->f_inode->i_uid = i->i_uid;
  f->f_inode->i_gid = i->i_gid;
  f->f_inode->i_size = i->i_size;
  f->f_inode->i_atime = i->i_atime;
  f->f_inode->i_mtime = i->i_mtime;
  f->f_inode->i_ctime = i->i_ctime;
  f->f_inode->i_blocks = i->i_blocks;
  for(int j=0; j<15; j++){
    f->f_inode->i_block[j] = i->i_block[j];
  }
  f->f_inode->i_op = i->i_op;
  f->f_inode->f_op = i->f_op;
  f->f_inode->i_sb = i->i_sb;
  f->f_inode->i_state = i->i_state;
  f->f_inode->i_flags = i->i_flags;
  return 0;
  
}
int ext2_release(struct inode *i, struct file *f){
  /* This function is called when a file is closed. It performs
   clean up operations if necessary. Returns zero for successful 
   operation.
  */
 free(root_inode);
 return 0;

}

//Read super_block and group descriptor
struct super_block *ext2_get_superblock(struct file_system_type *fs){
  struct ext2_inode ext2_root_inode;
  struct dentry *root_dentry = malloc(sizeof(struct dentry));
  root_inode = malloc(sizeof(struct inode));
  unsigned long long max_bytes; 

  //Read super block
  lseek(fs->file_descriptor, BASE_OFFSET, SEEK_SET);
  read(fs->file_descriptor, &superBlock, sizeof(superBlock));

  //For Global Variables
  block_size = 1024 << superBlock.s_log_block_size;
  read_group_descriptor(&myfs);
  inode_size = superBlock.s_inode_size;
  inodes_count = superBlock.s_inodes_count;
  blocks_count = superBlock.s_blocks_count;
  max_bytes = block_size*blocks_count;

  //Read ext2_root_inode and fill root_inode
  lseek(fs->file_descriptor, 
    BASE_OFFSET + (inode_table - 1)*block_size + sizeof(ext2_root_inode), SEEK_SET);
	read(fs->file_descriptor, &ext2_root_inode, sizeof(ext2_root_inode));
  root_inode->i_ino = 2;
  root_inode->i_mode = ext2_root_inode.i_mode;
  root_inode->i_nlink = ext2_root_inode.i_links_count;
  root_inode->i_uid = ext2_root_inode.i_uid;
  root_inode->i_gid = ext2_root_inode.i_gid;
  root_inode->i_size = ext2_root_inode.i_size;
  root_inode->i_atime = ext2_root_inode.i_atime;
  root_inode->i_mtime = ext2_root_inode.i_mtime;
  root_inode->i_ctime = ext2_root_inode.i_ctime;
  root_inode->i_blocks = ext2_root_inode.i_blocks;
  //i->i_state 
  root_inode->i_flags = ext2_root_inode.i_flags;
  for(int j=0; j<15; j++){
    root_inode->i_block[j] = ext2_root_inode.i_block[j];
  }
  root_inode->f_op = &f_op;
  root_inode->i_op = &i_op;
  root_inode->i_sb = &sup_block;

  //Fill Root Dentry
  root_dentry->d_flags = root_inode->i_flags;
  root_dentry->d_inode = root_inode;
  root_dentry->d_parent = NULL;
  root_dentry->d_name = malloc(strlen("/") + 1);
  strcpy(root_dentry->d_name, ".\n");    //CHECKKK root or /
  //root_dentry->d_fsdata ???
  root_dentry->d_sb = &sup_block;

  //Fill super block
  sup_block.s_inodes_count = superBlock.s_inodes_count;
  sup_block.s_blocks_count = superBlock.s_blocks_count;
  sup_block.s_free_blocks_count = superBlock.s_free_blocks_count;
  sup_block.s_free_inodes_count = superBlock.s_free_inodes_count;
  sup_block.s_first_data_block = superBlock.s_first_data_block;
  sup_block.s_blocksize = block_size;
  sup_block.s_blocksize_bits = superBlock.s_log_block_size;
  sup_block.s_blocks_per_group = superBlock.s_blocks_per_group;
  sup_block.s_inodes_per_group = superBlock.s_inodes_per_group;
  sup_block.s_minor_rev_level = superBlock.s_minor_rev_level;
  sup_block.s_rev_level = superBlock.s_rev_level;
  sup_block.s_first_ino = superBlock.s_first_ino;
  sup_block.s_inode_size = superBlock.s_inode_size;
  sup_block.s_block_group_nr = superBlock.s_block_group_nr;
  sup_block.s_maxbytes = max_bytes;
  sup_block.s_type = fs;
  sup_block.s_op = &s_op;
  sup_block.s_flags = root_inode->i_flags;
  sup_block.s_magic = superBlock.s_magic;
  sup_block.s_root = root_dentry;
  //sup_block.s_fs_info

  return &sup_block;
}

struct file_system_type *initialize_ext2(const char *image_path) {
  /* fill super_operations s_op */
  /* fill inode_operations i_op */
  /* fill file_operations f_op */
  /* for example:
      s_op = (struct super_operations){
        .read_inode = your_read_inode_function,
        .statfs = your_statfs_function,
      };
  */
  s_op = (struct super_operations){
        .read_inode = ext2_read_inode,
        .statfs = ext2_statfs,
  };
  i_op = (struct inode_operations){
        .lookup = ext2_lookup,    //not sure
        .readlink = ext2_readlink,
        .readdir = ext2_readdir,
        .getattr = ext2_getattr,
  };
  f_op = (struct file_operations){
        .llseek = ext2_llseek,
        .read = ext2_read,
        .open = ext2_open,
        .release = ext2_release,
  };
  myfs.name = fs_name;
  myfs.file_descriptor = open(image_path, O_RDONLY);
  /* assign get_superblock function
     for example:
        myfs.get_superblock = your_get_superblock;
  */
  myfs.get_superblock = ext2_get_superblock;
  return &myfs;
}
