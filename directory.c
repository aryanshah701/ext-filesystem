#include <string.h>
#include <bsd/string.h>
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>

#include "directory.h"
#include "util.h"
#include "inode.h"
#include "slist.h"

// typedef struct directory_ent {
//     char name[DIR_NAME];
//     int  inum;    
// } dirent;

int 
directory_init() 
{
    int rv;

    struct timespec curr_time;
    rv = clock_gettime(CLOCK_REALTIME, &curr_time);
    assert(rv >= 0);

    // Allocate a page and inode
    int inum = alloc_inode();
    int pnum = alloc_page();

    // No more inodes or pages left
    if (inum < 0 || pnum < 0) {
        fprintf(stderr, "Attempting to create a directory but no pages/inodes left");
        return -1;
    }

    // Initialize the inode
    inode* dir_inode = get_inode(inum);
    init_inode(dir_inode, 040755, pnum, 1, curr_time, 0);

    return inum;
}

int
get_num_dirs(inode* dd) 
{
    int size = dd->size;
    int num_dirs;

    if (size == 0) {
        num_dirs = 0;
    } else {
        num_dirs = dd->size / sizeof(dirent);
    }

    return num_dirs;
}

dirent*
get_dirents(inode* dd) 
{
    dirent* dirents = (dirent*)pages_get_page(dd->pnum[0]);
    return dirents;
}

int 
directory_lookup(inode* dd, const char* name)
{
    // Get the directory entries
    dirent* dirents = get_dirents(dd);
    int num_dirs = get_num_dirs(dd);

    for (int ii = 0; ii < num_dirs; ++ii) {
        if (streq(name, dirents[ii].name)) {
            return dirents[ii].inum;
        }
    }

    return -1;
}

int 
directory_lookup_idx(inode* dd, const char* name)
{
    // Get the directory entries
    dirent* dirents = get_dirents(dd);
    int num_dirs = get_num_dirs(dd);

    for (int ii = 0; ii < num_dirs; ++ii) {
        if (streq(name, dirents[ii].name)) {
            return ii;
        }
    }

    return -1;
}

int 
tree_lookup(inode* root_inode, const char* path)
{   
    if (streq(path, "/")) {
        return 0;
    }

    slist* splits = s_split(path, '/');
    int inum = -1;
    inode* dd = root_inode;
    
    while (splits) {
        if (!streq(splits->data, "")) {
            inum = directory_lookup(dd, splits->data);

            if (inum < 0) {
                return -ENOENT;
            }

            dd = get_inode(inum);
        }

        splits = splits->next;
    }

    return inum;
}

int 
directory_put(inode* dd, const char* name, int inum)
{
    // Get the directory entries
    dirent* dirents = get_dirents(dd);

    // Ensure there is space in the page for another directory ent
    int num_dirs = get_num_dirs(dd);
    int max_dirs = 4096 / sizeof(dirent);
    if (num_dirs >= max_dirs) {
        // Indicate an error for now
        return -1;
    }

    // Make sure a file/directory with this name doesn't already exist
    if (directory_lookup(dd, name) != -1) {
        return -1;
    }
    
    // Add a new directory entry
    dirent* new_dirent = &dirents[num_dirs];
    strlcpy(new_dirent->name, name, DIR_NAME);
    new_dirent->inum = inum;

    // Increment size of directory inode
    dd->size += sizeof(dirent);

    return 0;
}

int
directory_rename(inode* dd, const char* fname, const char* to_fname)
{
    // Get the directory entries
    dirent* dirents = get_dirents(dd);
    int num_dirs = get_num_dirs(dd);

    // Ensure the file/dir exists
    int del_idx = directory_lookup_idx(dd, fname);
    if (del_idx < 0) {
        return -1;
    }

    dirent* new_dirent = &dirents[del_idx];
    strlcpy(new_dirent->name, to_fname, DIR_NAME);

    return 0;
}

// Removes a directory entry by shifting the elements to the left down
int
remove_dirent(dirent* dirents, int del_idx, int size) 
{
    if (del_idx >= size || del_idx < 0) {
        return -1;
    }

    int ii = del_idx;
    while (ii < size - 1) {
        dirents[ii] = dirents[ii + 1];
        ++ii;
    }

    return 0;
}

int
directory_delete_all(inode* dd)
{
    // Get the directory entries
    dirent* dirents = get_dirents(dd);
    int num_dirs = get_num_dirs(dd);

    for (int ii = 0; ii < num_dirs; ++ii) {
        dirent entry = dirents[ii];
        char* name = entry.name;
        directory_delete(dd, name);
    }
}

int
directory_delete(inode* dd, const char* name)
{
    assert(name != 0);
    assert(dd != 0);

    // Get the directory entries
    dirent* dirents = get_dirents(dd);
    int num_dirs = get_num_dirs(dd);

    // Ensure the file/dir exists
    int del_idx = directory_lookup_idx(dd, name);
    int inum = directory_lookup(dd, name);

    if (del_idx < 0) {
        return -1;
    }

    inode* dir_inode = get_inode(inum);

    // If it does, then remove the entry
    int rv = remove_dirent(dirents, del_idx, num_dirs);
    assert(rv != -1);

    if (dir_inode->is_directory) {
        // directory_delete_all(dir_inode);
    }

    // Decrement size of directory inode
    dd->size -= sizeof(dirent);

    return rv;
}

void 
print_directory(inode* dd)
{
  // Get the directory entries
  dirent* dirents = get_dirents(dd);
  int num_dirs = get_num_dirs(dd);

  printf("Printing directories\n");
  for (int ii = 0; ii < num_dirs; ++ii) {
    printf("Name: %s, Inum: %d\n", dirents[ii].name, dirents[ii].inum);
  }
  printf("Finished printing directories\n");
}

int 
get_parent_inum(inode* root_inode, const char* path)
{
    // Get the name of the parent dir
    char* path_cpy = strdup(path);
    char* parent_dir_name = dirname(path_cpy);

    // Lookup the parent dir in root
    int dir_inum = tree_lookup(root_inode, parent_dir_name);
    return dir_inum;
}
