/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/
//#include "config.h"
#include "params.h"
#include "block.h"
//#include "fuse.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

#define BLOCK_SIZE 512       //2^9
#define FILESIZE 16777216    //2^24
#define INODE_COUNT 128      //2^7
#define MAX_FILE_SIZE 8192   //2^13
#define MAX_DATA_SIZE 1048576 //2^20 = inode * max file size
#define MAX_INODE_SIZE 65536 //2^16 = inode * 512
#define MAX_DATA_BLOCKS 16    //2^4 = 8192 / 512
#define MAX_FILE_NAME 256
#define DIRECT_INODE_COUNT 16
#define INDIRECT_INODE_COUNT 3
#define DOUBLE_INDIRECT_INODE_COUNT 2
#define INODE_LIST_START 19
#define TOTAL_DBLOCK_COUNT 2048 //2^20 / 2^9


struct sfs_state* sfsData;

typedef enum _bool{
  FALSE, TRUE
}bool;

typedef struct _bootBlock{
  int magicNum;
  int isinit;
  int superBlockIndex;
  //char bufSpace[500]; //filter space
}bootBlock;

typedef enum _blockType{
  ERROR, DIRECTORY, FILES
}blockType;

// 16 of them
typedef struct _inode{
  blockType type;
  struct stat s;
  int* indexes;
  int size;
  char name[MAX_FILE_NAME];

}inode;

// 3 of them pointing to 16 ea = 48
typedef struct _singleIndirectionInode{
  inode* list;
}singleIndirectionInode;

// 2 of them pointing to 2 single to 16 ea = 64
typedef struct _doubleIndirectionInode{
  singleIndirectionInode* list;
}doubleIndirectionInode;

typedef struct _dataBlock{
  int size;
  char data[BLOCK_SIZE - sizeof(int)];
}dataBlock;

typedef struct _superBlock{
  int partitionSize;
  int blocksize;
  inode* inodeList;
  singleIndirectionInode* singleList;
  doubleIndirectionInode* doubleList;
  bool* inode_bitmap;
  bool* data_bitmap;
}superBlock;

bootBlock* bBlock;
superBlock* sBlock;

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

// returns actual block value for inode given a bitmap index and type
int getBlockVal(int i, bool type){

  if(i < 0){
    return -1;
  }

  //FALSE type = inode
  if(type == FALSE){
    //direct list
    if(i < 16){
      return (i+19);
    }
    //single indirect list
    else if((i > 15) && (i < 64)){
      return (i+22);
    }
    //double indirect list
    else if((i > 63) && (i<128)){
      return (i+28);
    }
  }

  //TRUE type = data
  if(type == TRUE){

  }
}

// returns inode block index given the pathname of the file/dir
int getBlock(const char* path){
  int i,len,dataIndex, endIndex, pathIndex = 0;
  len = 0;
  // get indexes of the pathname set
  if((path == NULL) || (strlen(path) == 0)){
    return -1;
  }

  if(path[0] == '/'){
    pathIndex == 1;
    len++;
    if(strlen(path) == 1){
      return -1;
    }
  }

  if(path[strlen(path)-1] == '/'){
    endIndex = strlen(path)-2;
    len++;
  }
  else{
    endIndex = strlen(path)-1;
  }

  char* name = (char*)malloc((strlen(path) - len)*sizeof(char));

  // get bitmap to read thru inodes faster
  bool* inodes = (bool*)malloc(sizeof(bool)*INODE_COUNT);
  block_read(2,inodes);


  inode* tmpInode = (inode*)malloc(sizeof(inode));
  // traverse inode bitmap for name match
  for(i = 0; i < INODE_COUNT; i++){
    if(inodes[i] == TRUE){                 //check if it exists
      dataIndex = getBlockVal(i,FALSE);    //get actual block index
      block_read(dataIndex,tmpInode);     //get that shit
      if(strcmp(tmpInode->name,name) == 0){
        return i;
      }
    }
  }
  return -1;
}

// find the next free inode from the bitmap and returns its index
int findFreeInode(){
  // get bitmap to read thru inodes faster
  bool* inodes = (bool*)malloc(sizeof(bool)*INODE_COUNT);
  block_read(2,inodes);
  int i;

  inode* tmpInode = (inode*)malloc(sizeof(inode));
  // traverse inode bitmap for name match
  for(i = 0; i < INODE_COUNT; i++){
    if(inodes[i] == FALSE){                 //check if it is empty
      return i;    //return block index
    }
  }
  return -1;
}

// updates inode bitmap
int updateInodeBitmap(bool type, int block_index){

  bool* inodes = (bool*)malloc(sizeof(bool)*INODE_COUNT);
  block_read(2,inodes);

  if(type == TRUE){
    inodes[block_index] = TRUE;
  }
  else{
    inodes[block_index] = FALSE;
  }

  block_write(2,inodes);
  return 0;


}

//find which first block is free, returns -1 if none
int findFreeDataBlock(int inode_val, int startBlock){
    // get inode
    inode* tmpInode = (inode*)malloc(sizeof(inode));
    block_read(inode_val,tmpInode);


    // get data bitmap
    bool* dataBlocks = (bool*)malloc(sizeof(bool)*TOTAL_DBLOCK_COUNT);
    int i, counter = 0;
    for(i = 3; i < 19; i++){
      block_read(i,dataBlocks + counter);
      counter += 512;
    }

    for(i = startBlock; i < 16; i++){
      if(dataBlocks[tmpInode->indexes[i]] == FALSE){
        return tmpInode->indexes[i]; //returns actual value of the dBlock
      }
    }

    return -1;

}



int updateDataBitmap(bool type, int dblock_index){
  bool* dataBlocks = (bool*)malloc(sizeof(bool)*TOTAL_DBLOCK_COUNT);
  int i, counter = 0;
  for(i = 3; i < 19; i++){
    block_read(i,dataBlocks + counter);
    counter += 512;
  }

  if(type == TRUE){
    dataBlocks[dblock_index] = TRUE;
  }
  else{
    dataBlocks[dblock_index] = FALSE;
  }
  counter = 0;
  for(i = 3; i < 19; i++){
    block_write(i,dataBlocks + counter);
    counter += 512;
  }
  return 0;
}

int* getFreeDataBlocks(int inode_index){
  int* blocks = (int*)malloc(sizeof(int)*16);
  int i;
  for(i = 0; i < 16; i++){
    blocks[i] = (i*16) + 156;
  }
  return blocks;

}

int getDblockIndex(int Dblock_val){
  return (Dblock_val - 156);
}
/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn){
    fprintf(stderr, "in bb-init\n");
    log_msg("\nsfs_init()\n");
    int i, j ,k;
    // do i need to load in the values after i restart it?

    // open the disk
    disk_open(sfsData->diskfile);

    // get boot block information
    bBlock = (bootBlock*)malloc(sizeof(bootBlock));
    block_read(0,bBlock);

    if((bBlock->magicNum == 1409) && (bBlock->isinit == 1)){
      // filesystem is initialized already
      //everything same as below but block_read()?
    }
    else{  // filesystem is not initialized

      //block counter
      int block_counter = 0;

      //init boot block
      bBlock->magicNum = 1409;
      bBlock->superBlockIndex = 1;
      bBlock->isinit = 1;
      block_write(block_counter,bBlock);
      block_counter++;

      //init super block
      sBlock = (superBlock*)malloc(sizeof(superBlock));
      sBlock->partitionSize = FILESIZE;
      sBlock->blocksize = BLOCK_SIZE;
      sBlock->inodeList = (inode*)malloc(sizeof(inode)*DIRECT_INODE_COUNT);
      sBlock->inode_bitmap = (bool*)malloc(sizeof(bool)*INODE_COUNT);
      sBlock->data_bitmap = (bool*)malloc(sizeof(bool)*TOTAL_DBLOCK_COUNT);
      sBlock->singleList = (singleIndirectionInode*)malloc(sizeof(singleIndirectionInode)*INDIRECT_INODE_COUNT);
      sBlock->doubleList = (doubleIndirectionInode*)malloc(sizeof(doubleIndirectionInode)*DOUBLE_INDIRECT_INODE_COUNT);
      block_write(block_counter,sBlock);
      block_counter++;


      //init inode bitmap
      memset(sBlock->inode_bitmap, 0, (sizeof(bool)*INODE_COUNT));
      block_write(block_counter,sBlock->inode_bitmap);
      block_counter++;

      //init data bitmap
      memset(sBlock->data_bitmap, 0, (sizeof(bool)*TOTAL_DBLOCK_COUNT));
      for(block_counter = 3; block_counter < 19; block_counter++){
          block_write(block_counter,&(sBlock->data_bitmap[((block_counter-3)*512)])); //pls work
      }


      //init direct inodes
      for(i = block_counter; i < 35; i++){
        sBlock->inodeList[i].size = 0;
        sBlock->inodeList[i].s.st_dev = 0;
        sBlock->inodeList[i].s.st_ino = 0;
        sBlock->inodeList[i].s.st_mode = S_IFREG;
        sBlock->inodeList[i].s.st_nlink = 0;
        sBlock->inodeList[i].s.st_uid = 0;
        sBlock->inodeList[i].s.st_gid = 0;
        sBlock->inodeList[i].s.st_rdev = 0;
        sBlock->inodeList[i].s.st_size = 0;
        sBlock->inodeList[i].s.st_blksize = 0;
        sBlock->inodeList[i].s.st_blocks = 0;
        sBlock->inodeList[i].s.st_atime = 0;
        sBlock->inodeList[i].s.st_mtime = 0;
        sBlock->inodeList[i].s.st_ctime = 0;
        block_write((i),&(sBlock->inodeList[i]));
      }

      block_counter = 38;

      //init indirect inodes
      for(i = 0; i < INDIRECT_INODE_COUNT; i++){
        sBlock->singleList[i].list = (inode*)malloc(sizeof(inode)*DIRECT_INODE_COUNT);
        block_write((i+35), &(sBlock->singleList[i]));
        for(j = 0; j < DIRECT_INODE_COUNT; j++){
          sBlock->singleList[i].list[j].size = 0;
          sBlock->singleList[i].list[j].s.st_dev = 0;
          sBlock->singleList[i].list[j].s.st_ino = 0;
          sBlock->singleList[i].list[j].s.st_mode = S_IFREG;
          sBlock->singleList[i].list[j].s.st_nlink = 0;
          sBlock->singleList[i].list[j].s.st_uid = 0;
          sBlock->singleList[i].list[j].s.st_gid = 0;
          sBlock->singleList[i].list[j].s.st_rdev = 0;
          sBlock->singleList[i].list[j].s.st_size = 0;
          sBlock->singleList[i].list[j].s.st_blksize = 0;
          sBlock->singleList[i].list[j].s.st_blocks = 0;
          sBlock->singleList[i].list[j].s.st_atime = 0;
          sBlock->singleList[i].list[j].s.st_mtime = 0;
          sBlock->singleList[i].list[j].s.st_ctime = 0;
          block_write(block_counter,&(sBlock->singleList[i].list[j]));
          block_counter++;
        }
      }

      //count is now 88 - beginning of double pointer's single
      block_counter = 88;
      int block_counter2 = 92;
      //init double indirect inodes
      for(i = 0; i < DOUBLE_INDIRECT_INODE_COUNT;i++){
        sBlock->doubleList[i].list = (singleIndirectionInode*)malloc(sizeof(singleIndirectionInode)*2);
        block_write((i+86),&(sBlock->doubleList[i]));
        for(j = 0; j < 2; j++){
          sBlock->doubleList[i].list[j].list = (inode*)malloc(sizeof(inode)*DIRECT_INODE_COUNT);
          block_write(block_counter,&(sBlock->doubleList[i].list[j]));
          block_counter++;
          for(k = 0; k < DIRECT_INODE_COUNT; k++){
            sBlock->doubleList[i].list[j].list[k].size = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_dev = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_ino = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_mode = S_IFREG;
            sBlock->doubleList[i].list[j].list[k].s.st_nlink = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_uid = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_gid = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_rdev = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_size = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_blksize = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_blocks = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_atime = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_mtime = 0;
            sBlock->doubleList[i].list[j].list[k].s.st_ctime = 0;
            block_write(block_counter2,&(sBlock->doubleList[i].list[j].list[k]));
            block_counter2++;
          }
        }
      }
    }

    log_conn(conn);
    log_fuse_context(fuse_get_context());

    return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata){


    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
    disk_close(sfsData->diskfile);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf){
    // log and init vars
    int inodeIndex,retstat = 0;
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

    // grab the inode index needed
    if((inodeIndex = getBlock(path)) == -1){
      return -1;
    }

    // read the inode from sfs
    inode* tmpInode = (inode*)malloc(sizeof(inode));
    block_read(inodeIndex,tmpInode);

    // fill in statbuf
    statbuf->st_dev = tmpInode->s.st_dev;
    statbuf->st_ino = tmpInode->s.st_ino;
    statbuf->st_mode = tmpInode->s.st_mode;
    statbuf->st_nlink = tmpInode->s.st_nlink;
    statbuf->st_uid = tmpInode->s.st_uid;
    statbuf->st_gid = tmpInode->s.st_gid;
    statbuf->st_rdev = tmpInode->s.st_rdev;
    statbuf->st_size = tmpInode->s.st_size;
    statbuf->st_blksize = tmpInode->s.st_blksize;
    statbuf->st_blocks = tmpInode->s.st_blocks;
    statbuf->st_atime = tmpInode->s.st_atime;
    statbuf->st_mtime = tmpInode->s.st_mtime;
    statbuf->st_ctime = tmpInode->s.st_ctime;

    free(tmpInode);
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    int retstat = 0;

    if(strlen(path) > MAX_FILE_SIZE){
      return ENAMETOOLONG;
    }

    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",path, mode, fi);


    int block_index = getBlock(path);
    if(block_index != -1){
      retstat = EEXIST;
    }
    else{
      if((block_index = findFreeInode()) == -1){
        log_msg("no free inodes found\n");
        return -1;
      }

      inode* node = (inode*)malloc(sizeof(inode));
      node->type = FILES;
      node->s.st_dev = 0;
      node->s.st_ino = 0;
      node->s.st_mode = S_IFREG;
      node->s.st_nlink = 0;
      node->s.st_uid = getuid();
      node->s.st_gid = getgid();
      node->s.st_rdev = 0;
      node->s.st_size = 0;
      node->s.st_blksize = 0;
      node->s.st_blocks = 0;
      node->s.st_atime = time(NULL);
      node->s.st_mtime = time(NULL);
      node->s.st_ctime = time(NULL);
      strcpy(node->name, path);
      node->indexes = getFreeDataBlocks(block_index);

      //write the newly created block in
      int block_val = getBlockVal(block_index, FALSE);
      block_write(block_val,node);

      //update bitmap
      updateInodeBitmap(TRUE, block_index);



    }
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path){
    int retstat = 0;
    if(strlen(path) > MAX_FILE_NAME){
      return ENAMETOOLONG;
    }
    log_msg("sfs_unlink(path=\"%s\")\n", path);

    int block_index = getBlock(path);
    if(block_index == -1){
      return -1;
    }
    int block_val = getBlockVal(block_index,FALSE);
    inode* node = (inode*)malloc(sizeof(inode));
    block_read(block_val,node);

    int i;
    char* emptyData = (char*)malloc((sizeof(char)*BLOCK_SIZE));
    memset(emptyData, '0', BLOCK_SIZE);
    for(i = 0; i < 16; i++){
      block_write(node->indexes[i],emptyData);
    }
    updateInodeBitmap(FALSE,block_index);


    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi){
    int retstat = 0;

    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",path, fi);

    if(strlen(path) > MAX_FILE_NAME){
		    return ENAMETOOLONG;
    }

    int block_val = getBlock(path);

    if(block_val == -1){
      log_msg("File does not exist\n");
      retstat = -ENOENT;
    }

    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi){
    int retstat = 0;
    if(strlen(path) > MAX_FILE_NAME){
        return ENAMETOOLONG;
    }
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",path, fi);


    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    int retstat = 0;
    if(strlen(path) > MAX_FILE_NAME){
        return ENAMETOOLONG;
    }
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",path, buf, size, offset, fi);

    int block_index = getBlock(path);
    if(block_index == -1){
      log_msg("File does not exist\n");
      return -ENOENT;
    }
    int block_val = getBlockVal(block_index,FALSE);
    int startBlock = offset / BLOCK_SIZE;
    int startIndex = offset % BLOCK_SIZE;
    inode* node = (inode*)malloc(sizeof(inode));
    char buffer[BLOCK_SIZE];
    memset(buffer,0,BLOCK_SIZE);
    block_read(block_val,node);

    if((16*BLOCK_SIZE - node->size) < size){
      log_msg("Read is out of bounds of file\n");
      return -1;
    }

    int bytes = 0;

    while(bytes < size){
      if(bytes == 0){
        block_read(node->indexes[startBlock], buffer);
        memcpy(buf, buffer+startIndex, BLOCK_SIZE - startIndex);
        bytes += (BLOCK_SIZE - startIndex);
      }
      else if( (size - bytes) < BLOCK_SIZE){
        block_read(node->indexes[startBlock], buffer);
        memcpy(buf + bytes, buffer, size-bytes);
        bytes = size;
      }
      else{
        block_read(node->indexes[startBlock], buffer);
        memcpy(buf + bytes, buffer, BLOCK_SIZE);
        bytes += BLOCK_SIZE;
      }
      startBlock++;
    }
    retstat = strlen(buf);
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
    int retstat = 0;
    if(strlen(path) > MAX_FILE_NAME){
        return ENAMETOOLONG;
    }
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",path, buf, size, offset, fi);

    int block_index = getBlock(path);
    if(block_index == -1){
      log_msg("File does not exist\n");
      return -ENOENT;
    }
    int block_val = getBlockVal(block_index,FALSE);
    int startBlock = offset / BLOCK_SIZE;
    int startIndex = offset % BLOCK_SIZE;
    inode* node = (inode*)malloc(sizeof(inode));
    char buffer[BLOCK_SIZE];
    memset(buffer,0,BLOCK_SIZE);
    block_read(block_val,node);

    //find availible space
    int i, dblock_val;
    if((dblock_val = findFreeDataBlock(block_val,startBlock)) == -1){
      log_msg("Size too big\n");
      return ENOSPC;
    }

    // can only make 16 writes then i suppose..
    int eofBlock = node->indexes[15];
    int blocksWritten = 0;
    for(i = dblock_val; i < eofBlock; i++){
      if(size > (blocksWritten*BLOCK_SIZE)){
        block_write(i,buf);
        node->size+= BLOCK_SIZE;
        updateDataBitmap(FALSE,getDblockIndex(i));
        blocksWritten++;
      }
    }

    return size;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode){
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);


    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path){
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);


    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi){
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);


    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,struct fuse_file_info *fi){
    int retstat = 0;
    //filler_t fuse   -  UNCLEAR FUSE FUNCTIONS
    log_msg("\nsfs_readdir: %s\n", path);
    char* name = malloc(strlen(path)+1);
    name[strlen(path)] = '\0';
    strcpy(name, path);



    filler(buf, name, NULL, offset);

    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi){
    int retstat = 0;


    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage(){
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[]){
    int fuse_stat;
    struct sfs_state *sfs_data;

    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    sfs_data->logfile = log_open();

    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

    return fuse_stat;
}
