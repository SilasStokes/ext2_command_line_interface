#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>


int enter_name(MINODE *pip, int ino, char *name) {
    int i = 0;
    char buf[BLKSIZE];

    for (; i < 12; i++){
        if (pip -> INODE.i_block[i] == 0) break;
        get_block(pip -> dev, (pip -> INODE.i_block[i]), buf);// not sure if this is the intent
        DIR * dp = (DIR * )buf;
        char *cp = buf;
        int need_length = 4 * ((8 + strlen(name)+ 3) /4); // this is a weird floor function
        if (DEBUG) printf("dp -> rec_len = 0x%x\n", dp ->rec_len );
        while (cp + dp->rec_len < buf + BLKSIZE){
            printf("dp -> name = %s\n", dp -> name);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        } 

        int ideal_length = 4 * ((8 + dp->name_len+ 3) /4); // this is a weird floor function
        // dp points at last entry in block
        int remain = dp -> rec_len - ideal_length;
        if (DEBUG) printf("line 30: remain =  %d\n" , remain);
        if (remain >= need_length) {
            
            ideal_length = 4 * ((8 + dp->name_len+ 3) /4); // this is a weird floor function
            if (DEBUG) printf("entered remain >= need_length, need_length =  %d\n", need_length  );
            dp -> rec_len = ideal_length ;
            if (DEBUG) printf("newRecords: dp -> rec_len %d\n",dp -> rec_len );
            // do I need to change the name and add like a null character or something?
            cp += dp -> rec_len;
            DIR * newDP = (DIR *)cp;
            newDP -> inode = ino;
            newDP -> rec_len = remain;
            newDP -> name_len = strlen(name);
            strncpy(newDP -> name, name, strlen(name));
            put_block(pip ->dev, pip ->INODE.i_block[i], buf);
            print_buf(buf);
            return 1;
        } 
    }

    int nBLK = balloc(pip->dev);
    pip -> INODE.i_size += BLKSIZE;
    pip -> INODE.i_block[i+1] = nBLK;
    get_block(pip->dev, nBLK, buf);

    DIR * newDP = (DIR*) buf;
    newDP -> inode = ino;
    newDP -> rec_len = BLKSIZE;
    newDP -> name_len = strlen(name);
    strncpy(newDP -> name, name, strlen(name));
    put_block(pip->dev, pip->INODE.i_block[i+1], buf);
    print_buf(buf);
    return 1;
}


/* pip = parent inode pointer, name = child's directory name */
void mymkdir(MINODE * pip, char *name) {
    MINODE *mip;

    int ino = ialloc(dev);
    if (DEBUG) printf("ialloc(dev) = %d\n" , ino);
    int bno = balloc(dev);
    if (DEBUG) printf("balloc(dev) = %d\n" , bno);

    // which one is it??
    mip = iget(dev,ino);       //         should      equal
    INODE *ip = &(mip->INODE);
    

    *ip  = (INODE) { // for some reason this syntax works and theo other doesnt.
        .i_mode = LINUX_S_IFDIR | 0755, // 0x41ED, 
        .i_uid = running->uid, 
        .i_gid = running->gid, 
        .i_size = BLKSIZE, 
        .i_links_count = 2, 
        .i_atime = time(0L), 
        .i_ctime = time(0L), 
        .i_mtime = time(0L), 
        .i_blocks = 2, 
        .i_block = {bno} 
    };
        
    mip->dirty = 1;               // mark minode dirty

    char buf[BLKSIZE];

    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    DIR *dp = (DIR *)buf;
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (DIR *)(((char *)dp) + 12);
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';

    put_block(dev, bno, buf);

    if (DEBUG) printf("\ncalling enter name\n");
    enter_name(pip, ino, name);
    iput(mip);
}

void make_dir(char *pathname){
    char child[32]; 
    char parent[128]; 
    int dev;
    tokenize(pathname); // gpath has directories, n is the amount
    if (pathname[0] == '/'){ // absolute
        if (DEBUG) printf("Directory being made from root\n" );
        dev = root -> dev;
        strcpy(parent, "/");
        if (n > 1) {
            strcat(parent, name[0]); 
        }
    } else {
        if (DEBUG) printf("Directory being made from cwd\n" );
        dev = running -> cwd -> dev;
        if(n > 1) {
            strcpy(parent, name[0]); 
        } else {
            strcpy(parent, ".");
        }
    }
    /* setting up parent and child */
    strcpy(child, name[n-1]);
    if (DEBUG) printf("child: %s\n", child);
    int i;
    for ( i = 1; i < n-1; i++) {
        strcat(parent, "/");
        strcat(parent, name[i]);
    }
    if (DEBUG) printf("parent: %s\n", parent);
    /* 
    3. Get minode of parent:
        pino  = getino(parent);
        pip   = iget(dev, pino); 
    Verify : (1). parent INODE is a DIR (HOW?)   AND
             (2). child does NOT exists in the parent directory (HOW?);k
     */ 
    int pino = getino(parent);
    if (DEBUG) printf("pino = getino(parent) =  %d\n" ,pino );
    MINODE *pip = iget(dev, pino);
    if (DEBUG) printf("MINODE *pip -> ino = %d\n" , pip->ino);

    if (!((pip->INODE.i_mode & 0xF000) == 0x4000))
        ERROR("Not a directory");

    if (search(pip, child) != 0)
        ERROR("directory already exists");

    /* TODO: Check child does not exist using search(pip, child); == 0*/
    mymkdir(pip, child);
    /*  
    inc parent inodes's links count by 1; 
    touch its atime, i.e. atime = time(0L), mark it DIRTY
    */
    pip -> INODE.i_links_count++; 
    pip -> refCount++; // this isn't in his writeup but makes sense
    pip -> INODE.i_atime = pip -> INODE.i_ctime = pip->INODE.i_mtime = time(0L);
    pip -> dirty = 1;
    iput(pip);
}

int create(char *pathname) {
    char *dirname, *basename;

    splitpath(pathname, &dirname, &basename);

    if (strcmp(basename, "") == 0)
        RERROR("Cannot creat directory");
    
    int pino = getino(dirname);

    if (pino == 0)
        RERROR("Connot find directory");
    
    MINODE *pmip = iget(dev, pino);

    if (!LINUX_S_ISDIR(pmip->INODE.i_mode))
        RERROR("Can only creat file inside a directory");
    
    if (search(pmip, basename) != 0)
        RERROR("File already exists");

    int ino = ialloc(dev);
    MINODE *mip = iget(dev, ino);
    INODE *in = &mip->INODE;
    time_t t = time(0L);
    *in  = (INODE) {
        .i_mode = 0644 | LINUX_S_IFREG,
        .i_uid = running->uid, 
        .i_gid = running->gid, 
        .i_size = 0, 
        .i_links_count = 1, 
        .i_atime = t, 
        .i_ctime = t, 
        .i_mtime = t, 
        .i_blocks = 0, 
        .i_block = {0} 
    };


    mip->dirty = 1;
    iput(mip);
    

    enter_name(pmip, ino, basename);

    pmip->dirty = 1;
    iput(pmip);

    return ino;
}
