#include "functions.h"
#include "globals.h"
#include <stdio.h>
#include <string.h>
#include "util.h"

int myread(int fd, char *buf, int nbytes){

    int count = 0;
    char *qp = buf;
    OFT * oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    int available = mip->INODE.i_size - oftp->offset;

    int dev = mip->dev;

    while(nbytes && available) {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        int blk;
        if (lbk < 12) {
            blk = mip->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) {
            unsigned int indirect_block[BLKSIZE / sizeof(unsigned int)];
            get_block(dev, mip->INODE.i_block[12], (char*) indirect_block);
            blk = indirect_block[lbk - 12];
        } else {
            unsigned int doubly_indirect_block[BLKSIZE / sizeof(unsigned int)];
            unsigned int indirect_block[BLKSIZE / sizeof(unsigned int)];
            get_block(dev, mip->INODE.i_block[13], (char*) doubly_indirect_block);

            int dibk = (lbk - 12 - 256) / (BLKSIZE / sizeof(unsigned int));

            get_block(dev, doubly_indirect_block[dibk], (char*) indirect_block);
            int ibk = (lbk - 12 - 256) % (BLKSIZE / sizeof(unsigned int));
            blk = indirect_block[ibk];
        }

        char readbuf[BLKSIZE];
        get_block(dev, blk, readbuf);

        char *cp = readbuf + startByte;
        int remain = BLKSIZE - startByte;
        
        int to_copy = nbytes;
        if (remain < to_copy)
            to_copy = remain;
        if (available < to_copy)
            to_copy = available;
        
        memcpy(qp, cp, to_copy);
        nbytes -= to_copy;
        available -= to_copy;
        count += to_copy;
        qp += to_copy;
        oftp->offset += to_copy;
    }
    // printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

void mycat(char *path) {
    char mybuf[BLKSIZE];
    int n;
    int fd = open_file(path, "0");

    if (fd < 0)
        return;

    while ((n = myread(fd, mybuf, BLKSIZE))) {
        fwrite(mybuf, sizeof(char), n, stdout);
    }

    myclose(path);
}