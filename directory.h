// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME 48

#include "inode.h"

typedef struct directory_ent {
    char name[DIR_NAME];
    int  inum;    
    char _reserved[12];
} dirent;

int directory_init();
int directory_lookup(inode* dd, const char* name);
int tree_lookup(inode* root_inode, const char* path);
int directory_put(inode* dd, const char* name, int inum);
int directory_rename(inode* dd, const char* fname, const char* to_fname);
int directory_delete(inode* dd, const char* name);
int directory_delete_all(inode* dd);
int get_parent_inum(inode* root_inode, const char* path);
void print_directory(inode* dd);

#endif
