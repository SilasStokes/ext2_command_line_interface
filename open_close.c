#include "functions.h"
#include "globals.h"
#include "util.h"
#include "type.h"
#include <stdlib.h>
#include <ext2fs/ext2fs.h>
#include <stdio.h>

int open_file(char *path, char *m){

    int mode = 0; 
    sscanf(m, "%d", &mode); //0 1 2 3 = R , W , RW , APPEND
    
    int ino = getino(path);
    if (ino ==0 ) {
        create(path);
        ino = getino(path);
    }

    if (*path == '/') {
        printf("root\n");
        dev = root -> dev;
    } else {
        printf("not root\n");
        dev = running -> cwd -> dev;
    }


    MINODE * mip = iget(dev, ino);
    
    if(!LINUX_S_ISREG(mip->INODE.i_mode))
        RERROR("Must be reg file");
    
    // check whether file is already open
    int i = 0;
    for (; i < NFD; i++) {
        if (running -> fd[i] == NULL) continue;
        if (running -> fd[i] -> mptr == mip && running -> fd[i] -> mode != 0) {
            RERROR("file already open in incompatible mode");
        }
    }

    /* initialize new OFT struct */
    OFT * oftp = malloc(sizeof(OFT));
    *oftp = (OFT) {
        .mode = mode,
        .refCount = 1,
        .mptr = mip,
        .offset = 0
    };

    /* set offset depending on mode */
    switch(mode){
        case 0: // read, offset = 0
            oftp -> offset = 0;
            break;
        case 1: // write, truncater file to 0 size
            truncate_inode(mip); 
            oftp -> offset = 0;
            break;
        case 2: // RW, do NOT truncatej
            oftp -> offset = 0;
            break;
        case 3:
            oftp -> offset = mip -> INODE.i_size; // append
            break;
        default:
            RERROR("invalid mode");
            return -1;
    }

    /* this is potential for a bug, I don't see that fd[i], ever gets explicitly set NULL */
    for( i = 0;running -> fd[i] != NULL; i++);
    running -> fd[i] = oftp; 

    time_t t = time(0L);
    mip -> INODE.i_atime = t;
    if (oftp -> mode != 0) {
        mip -> INODE.i_mtime = t;
    }

    return i;

}

int close_file(int fd){
    if (fd >= NFD || fd < 0) {
        RERROR("fd not in range");
    }
    if (running -> fd[fd] == NULL) {
        RERROR("running -> fd invalid");
    }

    OFT * oftp = running -> fd[fd];

    running -> fd[fd] = NULL;

    oftp -> refCount--;
    if (oftp -> refCount > 0) {
        return 0;
    }

    MINODE * mip = oftp -> mptr;
    iput(mip);
    free(oftp);
    return 0;
}

void myclose(char *path) {
    int ino = getino(path);
    MINODE * mip = iget(dev, ino);

    int i;
    for (i = 0; i < NFD; i++ ) {
        if (running -> fd[i] == NULL) 
            continue;

        if (running -> fd[i] -> mptr == mip) {
            close_file(i);
            return;
        }
    }
    ERROR("file not found");

}

int mylseek(int fd,unsigned int position) {
    if (fd >= NFD || fd < 0) {
        RERROR("fd not in range");
    }

    OFT * oftp = running -> fd[fd];

    
    if (oftp == NULL) {
        RERROR("fd invalid");
    }
    if (position > oftp -> mptr ->INODE.i_size )
        RERROR("position invalid");

    int original_offset = oftp -> offset;
    oftp -> offset = position;

    return original_offset;
}

int pfd() {
    int i = 0;
    printf("\tfd\tmode\toffset\tINODE\n");
    for (i = 0; i < NFD; i++) {
        if (running -> fd[i] == NULL) continue;

        printf("\t%d", i);
        switch(running -> fd[i] -> mode) {
            case 0:
                printf("\tREAD");
                break;
            case 1:
                printf("\tWRITE");
                break;
            case 2:
                printf("\tRW");
                break;
            case 3:
                printf("\tAPP");
                break;
        }
        printf("\t%d", running -> fd[i] ->offset);
        printf("\t[%d, %d]", running -> fd[i] -> mptr -> dev, running -> fd[i] -> mptr -> ino);
        printf("\n");
    }
    return 0;
}