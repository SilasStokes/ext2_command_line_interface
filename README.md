# EXT2 COMMAND LINE INTERFACE

### About:

This is a command line program meant to emulate a shell enviromnet that can do operations on multiple mounted EXT2 file systems. 

This is project somewhat more involved than it seems because instead of using the built in c libraries to interact with the ext2 file system, it all had to be written from scratch, for example instead of just, say, writing a file, the file would need to be individually written to INODE blocks in 1024 byte blocks, changing the INODE bit map for each allocated, embedding a `DIR` struct in the parents directory. As a result, extensive care had to be to read the [GNU documentation](https://www.nongnu.org/ext2-doc/ext2.html) and build the system from the ground up. 

I wrote this Spring 2021 for KC Wang's CS360 Systems Programming course with a partner. I subsequently TA'd the course for two semesters and with the two other TA's suppert (Zach and Emma), we were able to raise the average grade of the class 13%. 

### How it works:

Running the executable opens one of the ext2 disks in the repo. From there all commands are runnable. To test multiple mounting points use `mount /mnt disk2`.

### List of working commands: 
- mkdir
- cd   
- pwd
- creat 
- link
- unlink 
- symlink
- rmdir
- cat
- cp 
- open 
- pfd
- write 
- close 
- mount 
- stat
- chmod
- chown
- chgrp
- utime
- mv
- su