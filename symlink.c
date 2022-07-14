#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>

void symlink(char *target, char *linkname) {
    if (getino(target) == 0)
        ERROR("Target does not exist");
    if (getino(linkname) != 0)
        ERROR("Link already exists");
    
    int lino = create(linkname);

    if (lino < 0)
        return;

    MINODE *lmip = iget(dev, lino);
    lmip->INODE.i_mode &= ~LINUX_S_IFREG;
    lmip->INODE.i_mode |= LINUX_S_IFLNK;

    char buf[PATH_MAX];
    abspath(target, buf);
    // todo currently assuming path <= 60 chars
    size_t len = strlen(buf);
    lmip->INODE.i_size = len;
    strncpy((char*) lmip->INODE.i_block, buf, len);
    lmip->dirty = 1;
    iput(lmip);
}

void readlink(char *pathname) {
    int ino = getino(pathname);
    if(ino == 0)
        ERROR("No such file or directory");

    MINODE *mip = iget(dev, ino);

    if (!(mip->INODE.i_mode & LINUX_S_IFLNK))
        ERROR("Not a symbolic link");

    unsigned int len = mip->INODE.i_size;

    if (len <= 60) {
        // link is stored in the blocks array
        char *data = (char*) mip->INODE.i_block;
        for (unsigned int i = 0; i < len; i++) {
            putchar(data[i]);
        }
        putchar('\n');
    } else {
        // link is too large, and is stored in a block
        
        char buf[BLKSIZE];
        get_block(dev, mip->INODE.i_block[0], buf);
        buf[len] = '\0';
        puts(buf);
    }

    iput(mip);
}