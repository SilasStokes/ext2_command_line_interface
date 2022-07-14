/************* cd_ls_pwd.c file **************/
#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>

void chdir(char *pathname)
{
    printf("chdir %s\n", pathname);
    // READ Chapter 11.7.3 HOW TO chdir
    int ino = getino(pathname);
    if(ino == 0)
        ERROR("No such file or directory");
    MINODE *mip  = iget(dev, ino);
    if(!(mip->INODE.i_mode & LINUX_S_IFDIR))
        ERROR("Not a directory");
    iput(running->cwd);
    running->cwd = mip;

}

void ls_file(MINODE *mip, char *name)
{
    char ftime[64];
    int i;
    char *t1 = "xwrxwrxwr-------"; 
    char *t2 = "----------------";
    if ((mip->INODE.i_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c",'-');
    if ((mip->INODE.i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c",'d');
    if (( mip->INODE.i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c",'l'); 
    for (i=8; i >= 0; i--){
        if (mip->INODE.i_mode & (1 << i)) // print r|w|x 
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }
    printf("\t");                            // file permissions
    printf("%d\t", mip->INODE.i_links_count);// number of hard links
    printf("%d\t\t", mip->INODE.i_uid);        // owner name
    printf("%d\t", mip->INODE.i_gid);        // owner group
    if (mip -> INODE.i_size > 10) {
        printf("%d\t", mip->INODE.i_size);       // file size in bytes
    } else 
        printf("%d   \t", mip->INODE.i_size);       // file size in bytes
    time_t mtime = (time_t) mip->INODE.i_mtime;
    strcpy(ftime, ctime(&mtime));// print time in calendar form ftime[strlen(ftime)-1] = 0; 
    ftime[strlen(ftime)-1] = 0;// kill \n at end
    printf("%s\t", ftime);                   // time of modification
    printf("%s", name);                      // name

    if (LINUX_S_ISLNK(mip->INODE.i_mode)) {
        unsigned int len = mip->INODE.i_size;

        char path[PATH_MAX];
        bzero(path, PATH_MAX);

        if (len <= 60) {
            memcpy(path, mip->INODE.i_block, len);
            path[len] = 0;
        } else {
            // link is too large, and is stored in a block
            
            get_block(dev, mip->INODE.i_block[0], path);
            path[len] = 0;
        }

        printf(" -> %s", path);
    }

    printf("\n");                             
}

void myls_dir(MINODE *mip){

    printf("ino\tpermissions\tlinks\towner\tgroup\tbytes\t\ttime\t\t\tname\n");
    char buf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    get_block(dev, mip->INODE.i_block[0], buf); // dev is the "fd" or file descriptor, i_block is read into buffer
    dp = (DIR *)buf;
    cp = buf;
    while (cp < buf + BLKSIZE)
    {
        //printf("test, inode : %d\n", dp -> inode);
        strncpy(temp, dp->name, dp->name_len);
        //printf("name : %s", temp);
        temp[dp->name_len] = 0;
        printf("%d\t", dp -> inode);
        MINODE* mtemp = iget(dev, dp->inode);
        ls_file(mtemp, temp);
        iput(mtemp);
        //printf("dp -> rec_len = 0x%x\n", dp ->rec_len);
        cp += dp->rec_len; 
        dp = (DIR *)cp;
        //printf("in myls_dir loop, name: %s, cp = 0x%x, buf+BLKSIZE = 0x%x\n", temp, cp, buf+BLKSIZE);
    }
    //printf("outside myls_dir loop, cp = %p, buf+BLKSIZE = %p\n", cp, buf+BLKSIZE);

    printf("\n");
}

void ls(char *pathname) {
    int original_dev = dev;
    if (strcmp(pathname,"") != 0){
        int ino = getino(pathname);
        MINODE *mip = iget(dev, ino);
        if (!LINUX_S_ISDIR(mip -> INODE.i_mode )){
            tokenize(pathname);
            ls_file(mip, name[n-1]);
        } else
            myls_dir(mip);
        iput(mip);
    } else{
        printf("ls cwd\n");
        myls_dir(running->cwd);
    } 
    dev = original_dev;
}

void pwd()
{
    char buf[PATH_MAX];
    getpwd(buf);
    puts(buf);

}



