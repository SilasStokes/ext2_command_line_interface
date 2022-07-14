#ifndef UTIL
#define UTIL

#include "type.h"

#include <stdbool.h>

#include <sys/ptrace.h>
#define BREAKPOINT() asm("int $3") // Break in gdb. If not in gdb, crash.

MINODE *mialloc();

void midalloc(MINODE *mip);

void get_block(int dev, int blk, char *buf);
void put_block(int dev, int blk, char *buf);

void tokenize(char *pathname);

MINODE *iget(int dev, int ino);

void iput(MINODE *mip);

int search(MINODE *mip, char *name);

int getino(char *pathname);

void getpwd(char *buf);

void findmyname(MINODE *parent, u32 myino, char *myname);

u32 findino(MINODE *mip, u32 *myino); // myino = ino of . return ino of ..

int tst_bit(char *buf, int bit);

int set_bit(char *buf, int bit);

int clr_bit(char *buf, int bit);

void decFreeInodes(int dev);

MTABLE * get_mtable(int dev);

int ialloc(int dev);

int balloc(int dev);

int incFreeBlocks(int dev);

int incFreeInodes(int dev);

int bdalloc(int dev, int bno);

int idalloc(int dev, int ino);

void abspath(char *pathname, char *abs);

void splitpath(char *pathname, char** dir, char** base);

void chaselink(MINODE **mip);

void print_dir(char buf[BLKSIZE], DIR *dp);

void print_block(MINODE* pip, int block);

void print_buf(char buf [BLKSIZE]);

void truncate_inode(MINODE *);

// permissions checking

bool can_read(MINODE *);

bool can_write(MINODE *);

#endif