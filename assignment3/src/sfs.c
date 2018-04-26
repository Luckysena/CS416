/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/
#include "config.h"
#include "params.h"
#include "block.h"
#include "fuse.h"
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

//bitmap for data blocks is 16 * 128 * 4 = 8192 bytes so 16 blocks
//bitmap for inode is 128 * 4 = 512 bytes so 1 block
//superblock?? just an int?   cap of fsf, next free inode, size of DB
// superblock is metadata about entire fs check for open/read/write. can tell where next free db is or inode
// TIMESTAMP !!!!!!!!!!!!!!!!

//init - is it my fs, does it have data,
//make boot block at block 0 and point to super block and check it.

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
  int indexes[MAX_DATA_BLOCKS];
  char name[MAX_FILE_NAME];
  //char bufSpace[40];
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
  //char bufSpace[464];
}superBlock;

bootBlock* bBlock;
superBlock* sBlock;

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

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
void *sfs_init(struct fuse_conn_info *conn)
{
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
      sBlock->inode_bitmap = (bool*)malloc(sizeof(bool)*128);
      sBlock->data_bitmap = (bool*)malloc(sizeof(bool)*2048);
      sBlock->singleList = (singleIndirectionInode*)malloc(sizeof(singleIndirectionInode)*INDIRECT_INODE_COUNT);
      sBlock->doubleList = (doubleIndirectionInode*)malloc(sizeof(doubleIndirectionInode)*DOUBLE_INDIRECT_INODE_COUNT);
      block_write(block_counter,sBlock);
      block_counter++;


      //init inode bitmap
      memset(sBlock->inode_bitmap, 0, (sizeof(bool)*128));
      block_write(block_counter,sBlock->inode_bitmap);
      block_counter++;

      //init data bitmap
      memset(sBlock->data_bitmap, 0, (sizeof(bool)*2048));
      for(block_counter = 3; block_counter < 19; block_counter++){
          block_write(block_counter,&(sBlock->data_bitmap[((block_counter-3)*512)])); //pls work
      }


      //init direct inodes
      for(i = block_counter; i < 35; i++){
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
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);

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
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);


    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);


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
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);


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
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);


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
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);


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
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);


    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);


    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
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
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
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
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;


    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
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

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
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
