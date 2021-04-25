// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include "pages.h"

#define INODE_COUNT 96

typedef struct inode {
    mode_t mode; // permission & type
    int size; // bytes
    int pnum[12]; // direct page number
    int ind_pnum; // indirect page number
    int pages_used;
    short int is_directory; // is it a directory or file
    short int is_symlink; 
    int refs;
    time_t status_seconds;
    time_t viewed_seconds;
    time_t changed_seconds;
    char _reserved[28];
} inode;

void init_inode(inode* new_inode, mode_t mode, int pnum, int is_directory, struct timespec curr_time, int is_symlink);
void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);

#endif