#include <stdint.h>
#include <assert.h>
#include <sys/stat.h>

#include "inode.h"
#include "bitmap.h"


// typedef struct inode {
//     int refs; // reference count
//     int mode; // permission & type
//     int size; // bytes
//     int ptrs[2]; // direct pointers
//     int iptr; // single indirect pointer
// } inode;
//

void
init_inode(inode* new_inode, mode_t mode, int pnum, int is_directory, struct timespec curr_time, int is_symlink)
{
    new_inode->mode = mode;
    new_inode->size = 0;
    new_inode->pnum[0] = pnum;

    // Unused pages
    for (int ii = 1; ii < 12; ++ii) {
        new_inode->pnum[ii] = -1;
    }
    new_inode->ind_pnum = -1;
    
    new_inode->pages_used = 1;
    new_inode->refs = 1;
    new_inode->is_directory = is_directory;
    new_inode->is_symlink = is_symlink;
    new_inode->status_seconds = curr_time.tv_sec;
    new_inode->viewed_seconds = curr_time.tv_sec;
    new_inode->changed_seconds = curr_time.tv_sec;
}

void 
print_inode(inode* node) 
{
    printf("Printing INODE \n");
    printf("mode: %d\n", node->mode);
    printf("size: %d\n", node->size);
    printf("page num: %d\n", node->pnum[0]);
    printf("Inode printed\n");
}

inode* 
get_inode(int inum)
{
    assert(inum >= 0 && inum < INODE_COUNT);

    // Get the start of the inode
    inode* inodes = pages_get_page(1);
    
    return &inodes[inum];
}

int 
alloc_inode()
{
    // Find the first available inode and allocate that
    void* ibm = get_inode_bitmap();

    for (int ii = 0; ii < INODE_COUNT; ++ii) {
        if (!bitmap_get(ibm, ii)) {
            bitmap_put(ibm, ii, 1);
            printf("+ alloc_inode() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void 
free_inode(int inum)
{
    printf("+ free_inode(%d)\n", inum);
    void* ibm = get_inode_bitmap();
    bitmap_put(ibm, inum, 0);
}

int
grow_inode(inode* node, int size) 
{
    printf("+ grow_inode(%d)\n", size);

    if (size < node->size) {
        return -1;
    }

    node->size = size;
    return size;
}

int
shrink_inode(inode* node, int size)
{
    printf("+ shrink_inode(%d)\n", size);

    if (size > node->size) {
        return -1;
    }

    node->size = size;
    return size;
}

int 
inode_get_pnum(inode* node, int fpn)
{
    return 0;
}
