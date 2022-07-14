#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // this has O_RDWR

#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>


/*NOTES
    Need to implement the busy error checking for both functions, kinda brain spaced on how to do that atm. 
    Want to put MOUNT_COUNT in global so I can use it other places but the compiler freaks out when I do
        saying that it's defined in multiple places.
        Same with the "disk" global variable in main.c - 

*/
void print_mount_table(){
    int i;

    printf("%s, dev #: %d mounted on %s\n", "diskimage",dev, "/");
    for(i = 0; i < MOUNT_COUNT; i ++){
        printf("%s, dev #: %d mounted on %s\n", MountTable[i] -> devName, MountTable[i] -> dev, MountTable[i] -> mntName);
    }
}

int mount(char * filesys, char *mntpnt) {
    printf("mounting: %s, location: %s\n", filesys, mntpnt);

    int i = 0;
    char buf[BLKSIZE];
    /* 1. if no params, print current mount system */
    if (*filesys== 0 && *mntpnt == 0) {
        printf("print current mount\n");
        print_mount_table();
        return 0;
    }

    /*2: check if already mounted */
    for (i = 0; i < MOUNT_COUNT; i++) {
        if (strcmp(MountTable[i] -> devName, filesys) == 0) {
            RERROR("disk already mounted");
        }
    }

    /* 3. open file for RW, fd is new DEV */
    
    if ((fd = open(filesys, O_RDWR)) < 0) {
        RERROR("open %s failed\n");
    }

    MOUNT_COUNT++; // 
    MTABLE *nfile = (MTABLE*)malloc(sizeof(MTABLE));
    bzero(nfile, sizeof(MTABLE)); // you got me paranoid about garbage memory.

    nfile -> dev = fd;

    /********** read super block  ****************/
    get_block(nfile->dev, 1, buf);
    SUPER * newSP = (SUPER *)buf;
    /* verify it's an ext2 file system ***********/
    if (newSP->s_magic != 0xEF53) {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }

    /* 4. get ino and minode for mntpnt */
    int mnt_ino = getino(mntpnt);
    MINODE *mnt_mip = iget(dev, mnt_ino);

    /* 5. error checking the mount point */
    if (!LINUX_S_ISDIR(mnt_mip ->INODE.i_mode)){
        RERROR("Mount point is not a directory");
    }
//    if(mnt_mip ->INODE.i_links_count > 1) {
//        RERROR("Mount point is busy");
//    }
/*
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
*/
    /* 6. record new DEN in MountTable */
    nfile -> ninodes = newSP->s_inodes_count;
    nfile -> nblocks = newSP->s_blocks_count;
    nfile -> free_blocks = newSP -> s_free_blocks_count;
    nfile -> free_inodes = newSP -> s_free_inodes_count;

    char buf2[BLKSIZE];
    get_block(nfile -> dev, 2, buf2);
    GD* ngp = (GD *)buf2;

    log_block_size = sp->s_log_block_size;


    nfile -> bmap = ngp->bg_block_bitmap;
    nfile -> imap = ngp->bg_inode_bitmap;
    nfile -> iblock = ngp -> bg_inode_table;

    strcpy(nfile -> devName, filesys);
    strcpy(nfile -> mntName, mntpnt);

    nfile -> mntDirPtr = mnt_mip;
    
    mnt_mip ->mounted++;
    mnt_mip -> mptr = nfile;

    MountTable[MOUNT_COUNT- 1] = nfile;

    print_mount_table();
    return 0; // Great Success 

}

int umount(char * filesys) {
    int i;
    for (i = 0; i < MOUNT_COUNT; i++) {
        if (strcmp(MountTable[i] -> devName, filesys) == 0) {
            /* check to make sure there's no open files */

            MINODE * mip = MountTable[i] ->mntDirPtr;
            mip -> mounted = 0;
            iput(mip);
            return 0;
        }
    }
    RERROR("umount location not found");
}