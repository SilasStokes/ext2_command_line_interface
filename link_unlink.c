#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>


void unlink(char *pathname){
    int ino = getino(pathname);
    if (ino == 0) {
        ERROR("cannot unlink root");
    }

    MINODE * mip = iget(dev, ino);

    if (!can_write(mip))
        ERROR("Permission denied");

    if (LINUX_S_ISDIR(mip->INODE.i_mode))
        ERROR("Cannot unlink directory");


    mip -> INODE.i_links_count--;
    mip->dirty = 1;
    if (mip -> INODE.i_links_count == 0) {
        if (DEBUG) printf("link count is zero\n");
        truncate_inode(mip);
//        int j;
//        char buf[BLKSIZE];
//        bzero(buf, BLKSIZE);
//        for (j = 0; j < 12; j++) {
//            put_block(mip -> dev, mip -> INODE.i_block[j], buf);
//            bdalloc(mip -> dev, mip -> INODE.i_block[j]);
//            mip->INODE.i_block[j] = 0;
//        }
        idalloc(mip -> dev, ino);
    }
    
    char * dirname, *basename;
    splitpath(pathname, &dirname, &basename);

    iput(mip);
    

    int pino = getino(dirname);
    MINODE * pip = iget(dev, pino);

    rm_child(pip, basename);

    pip -> dirty = 1;
    pip -> INODE.i_links_count--;
    pip -> INODE.i_atime = pip -> INODE.i_mtime = time(0L);

    iput(pip);
}

void link(char *target, char *linkname) {
    int tino = getino(target);

    if (tino == 0)
        ERROR("No such file");
    
    MINODE *tmip = iget(dev, tino);

    if (LINUX_S_ISDIR(tmip->INODE.i_mode))
        ERROR("Cannot create hardlink to directory");
    
    if (getino(linkname) != 0)
        ERROR("File already exists");
    
    char *linkdir, *linkbase;
    splitpath(linkname, &linkdir, &linkbase);

    int pino = getino(linkdir);

    if(pino == 0)
        ERROR("Cannot create link in nonexistent directory");
    
    MINODE *pmip = iget(dev, pino);

    if(!LINUX_S_ISDIR(pmip->INODE.i_mode))
        ERROR("Cannot create link in non-directory");
    
    enter_name(pmip, tino, linkbase);

    tmip->INODE.i_links_count++;
    tmip->dirty = 1;
    iput(tmip);
    iput(pmip);
}
