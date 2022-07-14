 //stat,  chmod, utime;

#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>


void mystat(char * path) {
    char *parent, *child;

    int ino = getino(path);

    MINODE * mip = iget(dev, ino);
    splitpath(path, &parent, &child);
   	printf("name: %s\n", child);
	printf("size: %d\n", mip -> INODE.i_size);
	printf("# links: %d\n", mip -> INODE.i_links_count);
	printf("owner: %d\n", mip -> INODE.i_uid);
	printf("group id: %d\n", mip -> INODE.i_gid);
	printf("# blocks: %d\n", mip -> INODE.i_blocks);
	printf("ino: %d\n", mip->ino);
	printf("dev: %d\n", mip->dev);

    time_t atime = mip->INODE.i_atime, mtime = mip->INODE.i_mtime;

    printf("last accessed: %s", ctime(&atime));

	printf("last modified: %s", ctime(&mtime));

	iput(mip); 

}

void mychmod(char *path, char * mode){
    int ino = getino(path);
    MINODE * mip = iget(dev, ino);
    unsigned int permissions = 0; 
    sscanf(mode, "%o", &permissions);
    if (permissions == 0) {
        ERROR("ERROR: mode not given");
    }
    mip->INODE.i_mode &= 0777000;
    mip -> INODE.i_mode |= permissions;
    mip ->dirty = 1;


    iput(mip);

}

void mychown(char *path, char *owner) {
    int ino = getino(path);
    MINODE * mip = iget(dev, ino);
    int newowner = -1;
    sscanf(owner, "%d", &newowner);
    if (newowner <= -1) {
        ERROR("ERROR: new owner not given");
    }
    mip->INODE.i_uid = newowner;
    mip->dirty = 1;
    iput(mip);
}

void mychgrp(char *path, char *group) {
    int ino = getino(path);
    MINODE * mip = iget(dev, ino);
    int newgroup = -1;
    sscanf(group, "%d", &newgroup);
    if (newgroup <= -1) {
        ERROR("ERROR: new owner not given");
    }
    mip->INODE.i_gid = newgroup;
    mip->dirty = 1;
    iput(mip);
}

void myutime(char *path) {
    int ino = getino(path);
    MINODE *mip = iget(dev, ino);

    mip -> INODE.i_atime = time(0L);
    mip -> dirty = 1;

    iput(mip);
}

int mv(char * path1, char * path2) {
    /* make sure path1 exists */
    int ino = getino(path1);
    if (ino == 0) {
        RERROR("file does not exist");
    }

    /* make sure destination is clear */
    int dino = getino(path2);
    if (dino != 0) {
        RERROR("destination file already exists");
    }

    MINODE * mip = iget(dev, ino);

    char path2cpy[32];
    strcpy(path2cpy, path2);

    char *parentDir, *newFileName;
    splitpath(path2, &parentDir, &newFileName);
    int parent_ino = getino(parentDir);
    if (parent_ino == 0) {
        RERROR("new destination does not exist");
    }

    MINODE * parent_mip = iget(dev, parent_ino);

    if (parent_mip -> dev == mip -> dev) {
        if (DEBUG) printf("inside parent dev == nip -> dev\n");
        //link()
        link(path1, path2cpy);
        unlink(path1);
    } else {
        if (DEBUG) printf("dev doesnt match, copying file\n");
        mycp(path1, path2cpy);
        unlink(path1);
    }

    iput(mip);
    iput(parent_mip);

    return 0;
}

void su(char *uid, char *gid) {
    int u = -1, g = -1;
    sscanf(uid, "%d", &u);
    sscanf(gid, "%d", &g);

    if (u <= -1 || g <= -1)
        ERROR("invalid uid or gid");
    
    running->uid = u;
    running->gid = g;
}