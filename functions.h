#ifndef FUNCTIONS
#define FUNCTIONS

#include "type.h"

#define ERROR(message) do { puts(message); return; } while(0)
#define RERROR(message) do { puts(message); return -1; } while(0)

/* cd_ls_pwd.c */
void chdir(char *pathname);

void ls(char *pathname);

void ls_file(MINODE *mip, char *name);

void myls_dir(MINODE *mip);

void pwd();

/* mymkdir_creat.c */
int enter_name(MINODE *pip, int ino, char *name);

void mymkdir(MINODE * pip, char *name);

void make_dir(char *pathname);

int create(char *pathname);

/* rmdir.c */
void rm_child(MINODE *pip, char *dir_name) ;

int rm_dir(char *pathname);

/* link_unlink.c */
void link(char *target, char *linkname);

void unlink(char *pathname);

/* symlink.c */
void symlink(char *target, char *linkname);

void readlink(char *pathname);


/* minor_func.c */
void mystat(char * path);

void mychmod(char *path, char * mode);

void mychown(char *path, char *owner);

void mychgrp(char *path, char *group);

void myutime(char *path); 

void su(char *uid, char *gid);

/* level2.c */
int open_file(char *path, char * mode);

int close_file(int fd);
void myclose(char *path);
int mylseek(int fd, unsigned int position);

int pfd();

int myread(int fd, char *buf, int nbytes);

int mywrite(int fd, char *buf, int nbytes);

void mycat(char *path);

void mycp(char *path, char *copy);

int mv(char *path1, char *path2);

/* level 3 */
int mount(char *path1, char *path2);
int umount(char *);
#endif