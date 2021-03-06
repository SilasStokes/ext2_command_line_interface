#ifndef TYPE
#define TYPE

#include <ext2fs/ext2_fs.h>

/*************** type.h file ************************/
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc GD;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER   *sp;
GD      *gp;
INODE   *ip;
DIR     *dp;

#define FREE 0
#define READY 1

#define BLKSIZE 1024
#define NMINODE 128
#define NFD 16
#define NPROC 2

typedef struct minode
{
    INODE INODE;
    int dev, ino;
    int refCount;
    int dirty;
    int mounted;
    struct mtable *mptr;
} MINODE;

typedef struct oft
{
    int mode;
    int refCount;
    MINODE *mptr;
    int offset;
} OFT;

typedef struct proc
{
    struct proc *next;
    int pid;
    int ppid;
    int status;
    int uid, gid;
    MINODE *cwd;
    OFT *fd[NFD];
} PROC;


typedef struct mtable{
    int dev; // device number; 0 for FREE
    int ninodes; // from superblock
    int nblocks;
    int free_blocks; // from superblock and GD
    int free_inodes;
    int bmap; // from group descriptor
    int imap;
    int iblock; // inodes start block
    MINODE *mntDirPtr; // mount point DIR pointer
    char devName[64]; //device name
    char mntName[64]; // mount point DIR name
} MTABLE;

#endif