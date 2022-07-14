/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include "globals.h"
#include "util.h"
#include "functions.h"


//char *disk = "diskimage"; /* I want to put this is globals.h but compiler throws a fit*/

void quit();
void init()
{
    int i, j;
    MINODE *mip;
    PROC *p;
    printf("init()\n");
    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mptr = 0;
    }
    for (i = 0; i < NPROC; i++)
    {
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;

        p->cwd = 0;
        p->status = FREE;
        for (j = 0; j < NFD; j++)
            p->fd[j] = 0;
    }
}

// load root INODE and set root pointer to it
void mount_root()
{
    printf("mount_root()\n");
    root = iget(dev, 2);
}


void help() {
    puts("MENU: \n\tls\tcd\tpwd\tquit\nlvl 1\tcreat\tmkdir\trmdir");
    puts("\tlink\tunlink\tsymlink\treadlink");
    printf("\tstat\tchmod\tchown\tchgrp\tutime\n");
    printf("lvl 2\topen\tclose\tcat\tcp\tpfd\tmv\n");
    printf("lvl 3\tmount\tumount\tsu\n");
}

int main(int argc, char *argv[])
{
    MOUNT_COUNT = 0;
    disk = "diskimage";
    if (argc > 1) {
        disk = argv[1];
    }
    char buf[BLKSIZE];
    char line[128], cmd[32], pathname[128], pathname2[128];

    if (argc > 1)
        disk = argv[1];

    printf("checking EXT2 FS ....");
    if ((fd = open(disk, O_RDWR)) < 0)
    {
        printf("open %s failed\n", disk);
        exit(1);
    }
    dev = fd;
    root_dev = fd;

    /********** read super block  ****************/
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    /* verify it's an ext2 file system ***********/
    if (sp->s_magic != 0xEF53)
    {
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }
    printf("EXT2 FS OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;
    if (DEBUG) printf("sp -> blocks_count =  %d\n" ,nblocks );

    log_block_size = sp->s_log_block_size;

    get_block(dev, 2, buf);
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

    init();
    mount_root();
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->status = READY;
    running->cwd = iget(dev, 2);
    bzero(running->fd, sizeof(running->fd));
    printf("root refCount = %d\n", root->refCount);

    while (1)
    {
        printf("enter command or help: ");
        fgets(line, 256, stdin);
        line[strlen(line) - 1] = 0;

        if (line[0] == 0)
            continue;
        pathname[0] = 0;
        pathname2[0] = 0;

        sscanf(line, "%s %s %s", cmd, pathname, pathname2);
        printf("cmd=%s pathname=%s pathname2=%s\n", cmd, pathname, pathname2);

        if (strcmp(cmd, "ls") == 0)
            ls(pathname);
        else if (strcmp(cmd, "cd") == 0)
            chdir(pathname);
        else if (strcmp(cmd, "pwd") == 0)
            pwd();
        else if (strcmp(cmd, "mkdir") == 0) {
            if (DEBUG) printf("Entered mkdir if statement.\n");
            make_dir(pathname);
        }
        else if (strcmp(cmd, "rmdir") == 0) {
            if (DEBUG) printf("Entered rmdir if statement.\n");
            rm_dir(pathname);
        }
        else if (strcmp(cmd, "creat") == 0)
            create(pathname);
        else if (strcmp(cmd, "link") == 0)
            link(pathname, pathname2);
        else if (strcmp(cmd, "unlink") == 0)
            unlink(pathname);
        else if (strcmp(cmd, "symlink") == 0)
            symlink(pathname, pathname2);
        else if(strcmp(cmd, "readlink") == 0)
            readlink(pathname);
        else if(strcmp(cmd, "stat") == 0)
            mystat(pathname);
        else if(strcmp(cmd, "chmod") == 0)
            mychmod(pathname2, pathname);
        else if(strcmp(cmd, "chown") == 0)
            mychown(pathname, pathname2);
        else if(strcmp(cmd, "chgrp") == 0)
            mychgrp(pathname, pathname2);
        else if(strcmp(cmd, "utime") == 0)
            myutime(pathname);

        /* level 2 */
        
        else if(strcmp(cmd, "open") == 0)
            open_file(pathname, pathname2);
        else if(strcmp(cmd, "close") == 0)
            myclose(pathname);
        else if(strcmp(cmd, "cat") == 0)
            mycat(pathname);
        else if(strcmp(cmd, "cp") == 0)
            mycp(pathname, pathname2);
        else if(strcmp(cmd, "pfd") == 0)
            pfd();
        else if(strcmp(cmd, "mv") == 0)
            mv(pathname, pathname2);

        /* level 3 */
        else if(strcmp(cmd, "mount") == 0)
            mount(pathname, pathname2);
        else if(strcmp(cmd, "umount") == 0)
            umount(pathname);
        else if(strcmp(cmd, "su") == 0)
            su(pathname, pathname2);

        else if(strcmp(cmd, "devino") == 0){
            printf("global var's are: root_dev = %d, dev = %d, ino = %d\n", root_dev, dev, running -> cwd->ino);
        }
        else if (strcmp(cmd, "dev") == 0) {
            int x = 0;
            sscanf(pathname, "%d", &x);
            dev = x;
        }
        else if (strcmp(cmd, "help") == 0)
            help();
        else if (strcmp(cmd, "quit") == 0)
            quit();

        else
            puts("COMMAND NOT FOUND");
    }
}

void quit()
{
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if (mip->refCount > 0)
            iput(mip);
    }
    exit(0);
}
