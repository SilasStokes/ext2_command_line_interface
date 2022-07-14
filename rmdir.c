
#include "functions.h"

#include "globals.h"
#include "type.h"
#include "util.h"

#include <ext2fs/ext2fs.h>
#include <linux/limits.h>


/* bug in code: When removing a directory added using the mkdir command, the while loop will run infinitely
   But if another directory is added after using mkdir in the buffer, then the previous buffer can be removed.
   I think there is a problem in the mkdir function that isn't adjusting rec_len for the last entry properly  */

void rm_child(MINODE *pip, char *dir_name) {
    if (DEBUG) printf("entered rm_child with pip -> ino: %d and dir_name %s\n" ,pip ->ino, dir_name);
    char buf [BLKSIZE];
    DIR * dp, *prev_dp;
    char *cp, *prev_cp;
    int i;
    for (i = 0; i < 12; i++) {
        get_block(dev, (pip -> INODE.i_block[i]), buf);// not sure if this is the intent
        dp = (DIR * )buf;
        cp = buf;
        char temp[64];

        if (DEBUG) print_buf(buf);//pip, pip -> INODE.i_block[i]);
        
        while (cp < buf + BLKSIZE){
			strncpy(temp, dp->name, dp->name_len);
            temp[dp -> name_len] = 0;
            //if (DEBUG) printf("cp + dp->rec_len = %x, buf+BLKSIZE = %x, difference = %ld\n" ,(cp + dp->rec_len), (buf+BLKSIZE), (buf + BLKSIZE) - (cp + dp -> rec_len) );
            //print_dir(buf, dp);
            //getchar();
            //char *dpname = strip(temp, strlen(dp ->name), 0);
            if (strcmp(dir_name, temp) == 0){
                // three cases here
                printf("found directory, evaluating 3 cases\n");

                /* case 1: first block */

                if (cp == buf && dp -> rec_len == BLKSIZE) {

                    if (DEBUG) printf("case 1: first and only bloc\n");
                    memset(cp, 0,BLKSIZE); // set index to zero
                    bdalloc(pip -> dev, pip -> INODE.i_block[i]);

                    pip -> INODE.i_size-= BLKSIZE;
                    /* moving all i_blocks[>i] down an index */
                    int j;
                    for (j = i+1; j < 12; j++) {
                        char buf2[BLKSIZE];
                        get_block(pip -> dev, (pip -> INODE.i_block[j]), buf2);// not sure if this is the intent
                        put_block(pip -> dev, pip -> INODE.i_block[j-1], buf2);

                        if (j == 11 ) {
                            bzero(buf2, BLKSIZE);
                            put_block(pip -> dev, pip -> INODE.i_block[j], buf2);
                        }
                    }
                    pip->INODE.i_block[11] = 0;
                }



                /* case 3: last block entry */
                else if ((cp + dp -> rec_len) == (buf + BLKSIZE)  ){
                    if (DEBUG) printf("case 3: last block\n" );

                    int del_rec_len = dp -> rec_len;
                    prev_dp = (DIR *) prev_cp;
                    memset(dp, 0, del_rec_len);
                  
                    prev_dp -> rec_len += del_rec_len;
                    put_block(pip -> dev, pip -> INODE.i_block[i], buf);
                    return;
                }  

                /* case 2: middle block */
                else if( dp -> name != 0 ){
                   if (DEBUG) printf("case 2: middle block\n" );
                   ls("");

                   /* need to find remain size after dp */
                   printf("BUFFER BEFORE MEMMOVE\n");
                   print_buf(buf);
                   int excess_reclen = dp -> rec_len;
                   prev_cp = cp;
                   prev_dp = (DIR*)cp;
                   while(cp + dp->rec_len < buf + BLKSIZE) {
                       // Advance pointer to the entry to move
                       cp += dp->rec_len;
                       dp = (DIR *)cp;
                   }
                   //                   (buf + BLKSIZE) - (cp_prev + extra) + 1);
                   size_t remaining_mem  = (buf+BLKSIZE) - (prev_cp + excess_reclen) + 1;
                   dp -> rec_len = dp -> rec_len + excess_reclen;
                   printf("updated final rec: Name = %s, rec_len: %d\n",dp -> name, dp -> rec_len);
                   //printf("BUFFER after updated rec_len\n");
                   //print_buf(buf);
                   memmove(prev_cp, prev_cp + excess_reclen, remaining_mem);
                   //memcpy(prev_cp, prev_cp + excess_reclen, remaining_mem);

                   printf("BUFFER AFTER MEMMOVE\n");
                   print_buf(buf);
                   put_block(dev, pip -> INODE.i_block[i], buf);
                   ls("");
                    return;
                }
                return;
            }

            prev_cp = cp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        } 
    }
}

int rm_dir(char *pathname) {
    
    char *dirname, *basename;
    int ino = getino(pathname); if (DEBUG) printf("rm_dir ino #: %d\n" , ino);
    if (ino == 0)
        RERROR("Cannot find directory");

   	if(strcmp(pathname, ".") == 0 || strcmp(pathname, "..") == 0){
        RERROR(". and .. cannot be deleted.");
		return -1;
	}

    splitpath(pathname, &dirname, &basename);
    if (DEBUG) printf("rm_dir pathname:  %s, basename: %s\n" , dirname, basename);
    if (strcmp(basename, "") == 0 || strcmp(dirname, "") == 0)
        RERROR("directory or path does not exist");

    MINODE *mip = iget(dev, ino);
    if (!LINUX_S_ISDIR(mip->INODE.i_mode)) {
        RERROR("Can only remove directory");
    }

    if (!can_write(mip)) {
        RERROR("Permission denied");
    }

    if (mip -> refCount != 1) {
        RERROR("dir is busy and cannot be removed.");
    }

    if (mip -> INODE.i_links_count > 2){
        RERROR("Directory is not empty and cannot be removed.");
    }
    /* making sure inode is empty */
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    dp = (DIR *)buf;
    cp = buf;
    int i;
    for (i = 0; i < 12; i++) {
        get_block(dev, mip->INODE.i_block[i], buf); // dev is the "fd" or file descriptor, i_block is read into buffer
        if (mip->INODE.i_block[i]==0)
            continue;
        while (cp < buf + BLKSIZE) {
            if ((strcmp(dp ->name, ".") != 0 ) &&  (strcmp(dp ->name, "..") != 0)) {
                RERROR("Directory is not empty and cannot be removed.");
                return -1;
            }
            cp += dp->rec_len; 
            dp = (DIR *)cp;
        }
    }

    /* deallocate disk blocks */
    for (i=0; i<12; i++){
        if (mip->INODE.i_block[i]==0)
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);

    iput(mip); //(which clears mip->refCount = 0);
    // get parent ino

    int pino = getino(dirname);
    if (DEBUG) printf("Parent ino:  %d\n" , pino);
    if (DEBUG) printf("mip -> dev =:  %d\n" , mip ->dev);
    MINODE * pip = iget(mip->dev, pino);     
    ls("");
    rm_child(pip, basename);
    
/* 
    
9. decrement pip's link_count by 1; 
    touch pip's atime, mtime fields;
    mark pip dirty;
    iput(pip);
    return SUCCESS;

*/
    pip ->INODE.i_links_count--;
    pip -> INODE.i_atime = pip -> INODE.i_mtime = time(0L);
    pip -> dirty = 1;
    /* from the text book, not his website */
//    bdalloc(mip -> dev, mip -> INODE.i_block[0]);
//    idalloc(mip -> dev, mip -> ino);
    // ls("");
    iput(pip);
    return 0;

}