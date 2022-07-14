#include "util.h"
#include "globals.h"

#include <stdio.h>
#include <string.h>
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <linux/limits.h>

/*********** util.c file ****************/

MINODE *mialloc(){  // allocate a FREE minode for use

    int i;
    for (i=0; i<NMINODE; i++){
        MINODE *mp = &minode[i];
        if (mp->refCount == 0){
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return 0;
}

void midalloc(MINODE *mip) // release a used minode
{
    mip->refCount = 0;
}


void get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk * BLKSIZE, 0);
    read(dev, buf, BLKSIZE);
}
void put_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk * BLKSIZE, 0);
    write(dev, buf, BLKSIZE);
}

void tokenize(char *pathname)
{
    // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
    // Code in Chapter 11.7.2
    char *s;
    strcpy(gpath, pathname);
    n = 0;
    s = strtok(gpath, "/");
    while(s){
        name[n++] = s;
        s = strtok(0, "/");
    }

}

MINODE *iget(int dev, int ino)
{
    //printf("in iget with dev = %d ino = %d\n", dev, ino);
    // return minode pointer of loaded INODE=(dev, ino)
    // Code in Chapter 11.7.2
    MINODE *mip;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];
    // serach in-memory minodes first
    //printf("NMINODES: %d\n", NMINODE);

    for (i=0; i < NMINODE; i++){
        MINODE *mip = &minode[i];
        //printf("%d - mip -> ino = %d, mip -> dev %d, refcount = %d\n", i, mip -> ino, mip -> dev, mip -> refCount);
        if (mip->refCount && (mip->dev==dev) && (mip->ino==ino)){
            mip->refCount++;
            return mip;
        }
    }
    // needed INODE=(dev,ino) not in memory
    mip = mialloc();
    // allocate a FREE minode
    mip->dev = dev; mip->ino = ino;
    // assign to (dev, ino)
    block = (ino-1)/8 + inode_start; //**
    // disk block containing this inode
    offset= (ino-1)%8;
    // which inode in this block
    get_block(dev, block, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip;
    // copy inode to minode.INODE
    // initialize minode
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;

}

void iput(MINODE *mip)
{
    // dispose of minode pointed by mip
    // Code in Chapter 11.7.2
    INODE *ip;
    int block, offset;
    char buf[BLKSIZE];
    if (mip==0) return;
    mip->refCount--;    // dec refCount by 1
    if (mip->refCount > 0) return;  // still has user
    if (mip->dirty == 0) return; // no need to write back

    // write INODE back to disk
    block = (mip->ino - 1) / 8 + inode_start; //** 
    offset = (mip->ino - 1) % 8;

    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset; // ip points at INODE
    *ip = mip->INODE; // copy INODE to inode in block
    put_block(mip->dev, block, buf); // write back to disk
    midalloc(mip); // mip->refCount = 0;

}

int search(MINODE *mip, char *name)
{
    // search for name in (DIRECT) data blocks of mip->INODE
    // return its ino
    // Code in Chapter 11.7.2

    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    for (i=0; i<12; i++){ // search DIR direct blocks only
        if (mip->INODE.i_block[i] == 0)
            return 0;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(name, temp)==0){
                printf("found %s : inumber = %d\n", name, dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;

}


MTABLE * get_mtable(int cur_dev) {
    int i = 0;

    for(i = 0; i < MOUNT_COUNT; i++) {
        if (MountTable[i] ->dev == cur_dev) {
            return MountTable[i];
        }
    }
    return (MTABLE *)NULL;
}

int getino(char * pathname) {
    printf("get ino called on %s", pathname);
    MINODE *mip;
    int i, ino;
    MTABLE *mn;
    int x =0;
    if (strcmp(pathname, "/")==0) {
        return 2;   // return root ino=2
    }
    if (pathname[0] == '/') {
        mip = iget(root_dev, 2);
    } else {
        mip = iget(running -> cwd ->dev, running -> cwd -> ino);
    }
    tokenize(pathname);

    for(i =0 ; i< n; i++) {
        ino = search(mip, name[i]);
        printf("in loop, ino = %d, name = %s\n", ino, name[i]);
        if (!ino){
            printf("no such component name %s\n", name[i]);
            iput(mip);
            return 0;
        }
        iput(mip);
        mip = iget(dev, ino);

        //if (!LINUX_S_ISDIR(mip->INODE.i_mode)){ // check DIR type
            //printf("%s is not a directory\n", name[i]);
            //iput(mip);
            //return 0;
        //}

        if ( mip -> dev != root_dev && mip -> ino == 2) {
            printf("upward traversal, x= %d\n", x );
                mn = get_mtable(dev);
                mip = mn -> mntDirPtr;
                dev = mip -> dev;
            x++;
        }
        else if (mip -> mounted) {
            mn = mip -> mptr;
            if (dev != mn -> dev) {
                dev = mn -> dev;
                mip = iget(dev, 2);
                ino = 2;
            }
        }
    }
    iput(mip);
    printf("exiting getino with ino = %d, dev = %d\n", ino , dev);
    return ino;
}

void rpwd(MINODE *wd, char *buf) {
    //if (wd == root)  {
    if (wd->dev == root_dev && wd ->ino == 2)  {
        return;
    }

    u32 parent_ino, my_ino;
    MINODE * pip;
    char lcl_name[257];
    if(wd ->ino == 2 && wd -> dev != root_dev) {
        MTABLE *mn = get_mtable(wd -> dev);
        parent_ino = mn ->mntDirPtr -> ino;
        my_ino = wd -> ino;
        pip = iget(mn -> mntDirPtr -> dev, parent_ino);

        rpwd(pip, buf);
    } else {
        parent_ino = findino(wd, &my_ino);
        pip = iget(wd -> dev, parent_ino);
        findmyname(pip, my_ino, lcl_name);

        rpwd(pip, buf);

        iput(pip);
        strcat(buf, "/");
        strcat(buf, lcl_name);
    }
}
void rpwd1(MINODE *wd, char *buf) {
    //if (wd == root)  {
    if (wd->dev == root_dev && wd ->ino == 2)  {
        return;
    }

    u32 parent_ino, my_ino;
    MINODE * pip;
    char lcl_name[257];
    if(wd ->ino == 2 && wd -> dev != root_dev) {
        printf("At the mounting directory\n");
        MTABLE *mn = get_mtable(wd -> dev);
        parent_ino = mn ->mntDirPtr -> ino;
        my_ino = wd -> ino;
        pip = iget(mn -> mntDirPtr -> dev, parent_ino);

        tokenize(mn -> mntName);
        strcpy(lcl_name, name[0]);

    } else {
        printf("called findino wd -> dev = %d, wd -> ino = %d\n", wd ->dev, wd -> ino);
        parent_ino = findino(wd, &my_ino);
        printf("in else branch of rwpd, parent_ino = %d\n", parent_ino);
        pip = iget(wd -> dev, parent_ino);
        findmyname(pip, my_ino, lcl_name);
    }
    printf("rpwd name = %s, pip -> ino = %d\n", lcl_name, pip -> ino);
    getchar();

    rpwd(pip, buf);

    iput(pip);
    strcat(buf, "/");
    strcat(buf, lcl_name);
}

void getpwd(char *buf) {
    if (running->cwd == root) {
        strcpy(buf, "/");
        return;
    }
    buf[0] = '\0';
    rpwd(running->cwd, buf);
}

void findmyname(MINODE *parent, u32 myino, char *myname)
{
    // TODO handle indirect blocks

    unsigned int num_blocks = parent->INODE.i_blocks / (2 << log_block_size);
    if (num_blocks > 12) {
        num_blocks = 12;
        // pretend indirect blocks don't exist for now
    }
    
    for(unsigned int i = 0; i < num_blocks; i++) {
        char buf[BLKSIZE];
        get_block(parent -> dev, parent->INODE.i_block[i], buf);

        DIR *entry = (DIR*) buf;
        while(entry->inode) {
            if(entry->inode == myino) {
                memcpy(myname, entry->name, entry->name_len);
                myname[entry->name_len] = '\0';
                return;
            } else {
                entry = (DIR *)(((char*) entry) + entry->rec_len);
            }
        }
    }
}

u32 findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
    // mip->a DIR minode. Write YOUR code to get mino=ino of .
    //                                         return ino of ..
    char buf[BLKSIZE];
    get_block(mip -> dev, mip->INODE.i_block[0], buf);

    *myino = * (u32 *) buf;
    u16 length = * (u16*) (buf + sizeof(u32));
    return * (u32 *) (buf + length);
}
int tst_bit(char *buf, int bit) {
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit){
    return buf[bit/8] |= (1 << (bit %8));
    //int i = bit / 8;
    //int j = bit % 8;
    //return (buf[i] |= (1<<j) );
}

int clr_bit(char *buf, int bit){
    return buf[bit/8] &= ~(1 << (bit%8));
}

void decFreeInodes(int dev) {
    // dec free inodes count in SUPER and GD
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

/** TODO:
 * QUESTION: the book says :
 *      In order to maintain file system consistency, allocating an inode must decrement the
 *      free inodes count in both superblock and group descriptor by 1.
 * however - this function does not do that. 
 */
// textbook ialloc
int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

// read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }
  return 0;
}


/*
Allocation of disk blocks is similar, except it uses the blocks bitmap and it decrements the free
blocks count in both superblock and group descriptor. This is left as an exercise.
Exercise 11.10 Implement the function
int balloc(int dev)
which allocates a free disk block (number) from a device.

## saw an issue in the discord about this function
    "my balloc() only reads from the global dv and bmap and not from the mount table.
     it worked well enough for the past levels that I never investigated on why my balloc 
     and ialloc didn't use the dev passed to them because when I tried to do what the book
     says - it doesn't work.
     the end result of that is that I only have nblocks of blocks to use and that runs out
     before the indirect blocks are done "
*/
/* returns free disk block number, not test/written or understood */

int balloc(int dev) {
    int i;
    char buf[BLKSIZE];
    get_block(dev, bmap, buf);
    for (i = 0; i < nblocks; i++){
        if (tst_bit(buf, i) == 0) {
            set_bit(buf, i);
            // TODO: decrement block count in super block and group descriptor
            //! QUESTION: this confuses me bc if you're allocating to super block, why are we decrementing?
            //sp -> s_free_blocks_count--;
            //if (DEBUG) printf("Free Block Count: %d\n" , sp -> s_free_blocks_count);
            // TODO: decremement group descriptor (?)
            //MTABLE *mp = (MTABLE *)get_mtable(dev);
            // mp -> freenodes --;
            put_block(dev, bmap, buf);
            printf("allocated block = %d\n", i+1); // bits count from 0; ino from 1
            return i+1;
        }
    }
    return 0;
}

int incFreeInodes(int dev) {
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
    return 0;
}

int incFreeBlocks(int dev) {
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp -> s_free_blocks_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
    return 0;
}

int bdalloc(int dev, int bno)
{
    char buf[BLKSIZE];
    if (bno > nblocks){ // nblocks global
        printf("inumber %d out of range\n", bno);
        return 0;
    }
    // get inode bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);
    // write buf back
    put_block(dev, bmap, buf);
    // update free inode count in SUPER and GD
    incFreeBlocks(dev);
    return 0;
}

//int bdalloc(int dev, int bno)
//{
//    int i;
//    char buf[BLKSZE];
//    MTABLE *mp = (MTABLE *)get_mtable(dev);
//    if (bno > mp->nblocks){ // nblocks global
//        printf("inumber %d out of range\n", bno);
//        return 0;
//    }
//    // get inode bitmap block
//    get_block(dev, mp->bmap, buf);
//    clr_bit(buf, bno-1);
//    // write buf back
//    put_block(dev, mp->bmap, buf);
//    // update free inode count in SUPER and GD
//    incFreeBlocks(dev);
//}

int idalloc(int dev, int ino)
{
    char buf[BLKSIZE];
    if (ino > ninodes){ // niodes global
        printf("inumber %d out of range\n", ino);
        return 0;
    }
    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    // write buf back
    put_block(dev, imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);
    return 0;
}
//int idalloc(int dev, int ino)
//{
//    int i;
//    char buf[BLKSIZE];
//    MTABLE *mp = (MTABLE *)get_mtable(dev);
//    if (ino > mp->ninodes){ // niodes global
//        printf("inumber %d out of range\n", ino);
//        return 0;
//    }
//    // get inode bitmap block
//    get_block(dev, mp->imap, buf);
//    clr_bit(buf, ino-1);
//    // write buf back
//    put_block(dev, mp->imap, buf);
//    // update free inode count in SUPER and GD
//    incFreeInodes(dev);
//}

void abspath(char *pathname, char *abs) {
    if(pathname[0] == '/') {
        strcpy(abs, pathname);
        return;
    }

    getpwd(abs);
    if (strcmp(abs, "/") != 0) {
        strcat(abs, "/");
    }
    strcat(abs, pathname);
}


// /a/b/c, dir is /a/b and c is base
// modifies pathname
void splitpath(char *pathname, char** dir, char** base) {

    //printf("%d", 42);

    *dir = NULL;
    *base = NULL;

    for(int i = strlen(pathname) - 1; i >= 0; i--) {
        if(pathname[i] != '/') {
            continue;
        }

        if(i == 0) {
            *dir = "/";
            *base = pathname + 1;
            break;
        }

        pathname[i] = '\0';
        *dir = pathname;
        *base = pathname + i + 1;
        break;
    }

    if (*dir == NULL) {
        *dir = ".";
        *base = pathname;
    }
}

void chaselink(MINODE **mip) {
    while (LINUX_S_ISLNK((*mip)->INODE.i_mode)) {

        unsigned int len = (*mip)->INODE.i_size;

        char path[PATH_MAX];
        bzero(path, PATH_MAX);

        if (len <= 60) {
            memcpy(path, (*mip)->INODE.i_block, len);
        } else {
            // link is too large, and is stored in a block
            
            get_block(dev, (*mip)->INODE.i_block[0], path);
        }

        int nino = getino(path);
        MINODE *next = iget(dev, nino);
        iput(*mip);
        *mip = next;
    }
}

void print_dir( char buf[BLKSIZE], DIR *dp) {
    char temp[64];
    strncpy(temp, dp->name, dp->name_len);
    temp[dp -> name_len] = 0;
    printf("name = \"%s\"\t", temp);
    printf("ino# = %d\t", dp ->inode);
    printf("rec_len = %d\t",dp->rec_len);
    printf("name_len = %d\t",dp->name_len);
    printf("file type = %x\t",dp -> file_type);
    printf("offset = %ld\t", (char*)dp - buf);
    printf("remaining space =  %ld\n", buf + BLKSIZE - ((char*)dp + dp ->rec_len));
}
void print_block(MINODE* pip, int block) {
    char buf[BLKSIZE];

    get_block(pip -> dev, block, buf);// not sure if this is the intent
    char *cp = buf;
    DIR* dp = (DIR * )buf;

    while (cp + dp->rec_len < buf + BLKSIZE){
        print_dir( buf, dp);
        cp += dp->rec_len;
        dp = (DIR *)cp;

    }
    printf("\n");
}
void print_buf(char buf [BLKSIZE]) {
    printf("printing current buffer: \n");
    char *cp = buf;
    DIR* dp = (DIR * )buf;

    while (cp + dp->rec_len < buf + BLKSIZE){
        print_dir( buf, dp);
        cp += dp->rec_len;
        dp = (DIR *)cp;

    }
    print_dir( buf, dp);
    printf("\n");
}



void truncate_inode(MINODE * mip){
    char buf[BLKSIZE];
    unsigned int ubuf[BLKSIZE / sizeof(unsigned int)];
    bzero(buf, BLKSIZE);
    long unsigned int i, j;
    /* direct blocks */
    for( i = 0; i < 12; i++){
        put_block(mip -> dev, mip -> INODE.i_block[i], buf);
        bdalloc(mip -> dev, mip -> INODE.i_block[i]);
        mip -> INODE.i_block[i] = 0;
    }

    /* indirect blocks */
    if (mip -> INODE.i_block[12] != 0) {

        get_block(mip -> dev, mip -> INODE.i_block[12], (char * ) ubuf);
        for (i = 0; i < BLKSIZE/sizeof(unsigned int); i++) {
            bdalloc(mip -> dev, ubuf[i]);
        }
        bzero(ubuf, BLKSIZE);
        put_block(mip ->dev, mip -> INODE.i_block[12], (char *) ubuf);
        bdalloc(mip -> dev, mip -> INODE.i_block[12]);
        mip -> INODE.i_block[12] = 0;
    }

    /* double indirect block */

    if (mip -> INODE.i_block[13] != 0) {

        get_block(mip -> dev, mip -> INODE.i_block[13], (char *) ubuf);

        for (i = 0; i < BLKSIZE/sizeof(unsigned int); i++) {
            //int j = 0;
            char ubuf2[BLKSIZE / sizeof(unsigned int)];

            get_block(mip -> dev, ubuf[i], (char *) ubuf2);

            for( j = 0 ;j < BLKSIZE / sizeof(unsigned int); j++) {
                bdalloc(mip -> dev, ubuf2[j]);
            }
            
        }

        bzero(ubuf, BLKSIZE);
        put_block(mip ->dev, mip -> INODE.i_block[13], (char *) ubuf);
        bdalloc(mip -> dev, mip -> INODE.i_block[13]);
        mip -> INODE.i_block[13] = 0;
    }

    mip -> INODE.i_size = 0;

    mip -> dirty = 1;
    //idalloc( pip -> dev, pip -> ino); // iffy on this being here. 

}

bool can_read(MINODE *mip) {
    unsigned int mode = mip->INODE.i_mode;

    if (running->uid == 0)
        return true;

    if (mode & LINUX_S_IROTH)
        return true;
    
    if (mip->INODE.i_gid == running->gid && mode & LINUX_S_IRGRP)
        return true;

    if (mip->INODE.i_uid == running->uid && mode & LINUX_S_IRUSR)
        return true;

    return false;
}

bool can_write(MINODE *mip) {
    unsigned int mode = mip->INODE.i_mode;

    if (running->uid == 0)
        return true;

    if (mode & LINUX_S_IWOTH)
        return true;
    
    if (mip->INODE.i_gid == running->gid && mode & LINUX_S_IWGRP)
        return true;

    if (mip->INODE.i_uid == running->uid && mode & LINUX_S_IWUSR)
        return true;

    return false;
}
