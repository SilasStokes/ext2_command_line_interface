
#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

int mywrite(int fd, char *buf, int nbytes){
    int count = 0;
    char *qp = buf;
    OFT * oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    mip->dirty = 1;
    int dev = mip->dev;

    while (nbytes) {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        int blk;

        char zbuf[BLKSIZE];
        int blocks_available = mip->INODE.i_blocks / (2 << log_block_size);

        printf("lbk: %d\n", lbk);
        if (lbk < 12) {
            if (lbk >= blocks_available) {
                mip->INODE.i_blocks += (2 << log_block_size);
                int bno = balloc(dev);
                mip->INODE.i_block[lbk] = bno;
                bzero(zbuf, BLKSIZE);
                put_block(dev, bno, zbuf);
            }

            blk = mip->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) {
            if (blocks_available <= 12) {
                int bno = balloc(dev);
                mip->INODE.i_block[12] = bno;
                bzero(zbuf, BLKSIZE);
                put_block(dev, bno, zbuf);
            }

            unsigned int indirect_block[BLKSIZE / sizeof(unsigned int)];
            get_block(dev, mip->INODE.i_block[12], (char*) indirect_block);

            if (lbk >= blocks_available) {
                mip->INODE.i_blocks += (2 << log_block_size);
                int bno = balloc(dev);
                indirect_block[lbk - 12] = bno;
                put_block(dev, mip->INODE.i_block[12], (char*) indirect_block);
                bzero(zbuf, BLKSIZE);
                put_block(dev, bno, zbuf);
            }

            blk = indirect_block[lbk - 12];
        } else {
            if (blocks_available <= 12 + 256) {
                int bno = balloc(dev);
                mip->INODE.i_block[13] = bno;
                bzero(zbuf, BLKSIZE);
                put_block(dev, bno, zbuf);
            }

            unsigned int doubly_indirect_block[BLKSIZE / sizeof(unsigned int)];
            unsigned int indirect_block[BLKSIZE / sizeof(unsigned int)];
            get_block(dev, mip->INODE.i_block[13], (char*) doubly_indirect_block);
            int dibk = (lbk - 12 - 256) / (BLKSIZE / sizeof(unsigned int));

            if (blocks_available <= 12 + 256 + dibk * 256) {
                int bno = balloc(dev);
                doubly_indirect_block[dibk] = bno;
                put_block(dev, mip->INODE.i_block[13], (char*) doubly_indirect_block);
                bzero(zbuf, BLKSIZE);
                put_block(dev, doubly_indirect_block[dibk], zbuf);
            }

            get_block(dev, doubly_indirect_block[dibk], (char*) indirect_block);
            int ibk = (lbk - 12 - 256) % (BLKSIZE / sizeof(unsigned int));

            if (lbk >= blocks_available) {
                mip->INODE.i_blocks += (2 << log_block_size);
                int bno = balloc(dev);
                indirect_block[ibk] = bno;
                put_block(dev, doubly_indirect_block[dibk], (char*) indirect_block);
                bzero(zbuf, BLKSIZE);
                put_block(dev, bno, zbuf);
            }

            blk = indirect_block[ibk];
        }

        char writebuf[BLKSIZE];
        get_block(dev, blk, writebuf);

        char *cp = writebuf + startByte;
        int remain = BLKSIZE - startByte;
        
        int to_copy = nbytes;
        if (remain < to_copy)
            to_copy = remain;
        
        memcpy(cp, qp, to_copy);
        nbytes -= to_copy;
        count += to_copy;
        qp += to_copy;
        put_block(dev, blk, writebuf);
        oftp->offset += to_copy;
    }

    if ((unsigned int) oftp->offset > mip->INODE.i_size) {
        mip->INODE.i_size = oftp->offset;
    }

    return count;
}

void mycp(char *path, char *copy){
    if (create(copy) < 0) {
        return;
    }
    char mybuf[BLKSIZE];
    int n;
    int rfd = open_file(path, "0");
    int wfd = open_file(copy, "1");

    if (rfd < 0 || wfd < 0) {
        return;
    }

    while ((n = myread(rfd, mybuf, BLKSIZE))) {
        mywrite(wfd, mybuf, n);
    }

    myclose(path);
    myclose(copy);
}
