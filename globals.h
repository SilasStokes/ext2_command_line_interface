#ifndef GLOBALS
#define GLOBALS

#include "type.h"

int MOUNT_COUNT;
MTABLE *MountTable[NPROC];

char *disk; //= "diskimage"; /* I want to put this is globals.h but compiler throws a fit*/
int root_dev;

MINODE minode[NMINODE];
MINODE *root;
#define DEBUG 1
PROC proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
char pname[128]; // global pathname
int n;           // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start, log_block_size;


#endif