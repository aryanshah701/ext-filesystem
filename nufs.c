// based on cs3650 starter code

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>
#include <libgen.h>
#include <time.h>

#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "util.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

const int ROOT_INODE = 0;
const int MAX_FILE_SIZE = 4096 * 12 * 1024;

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
    printf("Access\n");
    fflush(stdout);

    int rv = 0;
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
    printf("Getattr\n");
    fflush(stdout);

    int rv = 0;

    memset(st, 0, sizeof(stat));
    
    // Check if the file/dir exists
    inode* root_inode = get_inode(ROOT_INODE);
    int inum = tree_lookup(root_inode, path);

    if (inum < 0) {
        // File doens't exist
        rv = -ENOENT;
    } else if (strcmp(path, "/") == 0) {
        // Root directory
        st->st_mode = 040755;
        st->st_size = root_inode->size;
        st->st_uid = getuid();
    } else {
        // File/Dir in root directory
        inode* lookup_inode = get_inode(inum);
        
        st->st_ino = inum;
        st->st_mode = lookup_inode->mode;
        st->st_nlink = lookup_inode->refs;
        st->st_uid = getuid();
        st->st_gid = getgid();
        st->st_rdev = 0;
        st->st_size = lookup_inode->size;
        st->st_blksize = 4096;
        st->st_blksize = bytes_to_pages(lookup_inode->size);
        st->st_atime = lookup_inode->viewed_seconds;
        st->st_mtime = lookup_inode->changed_seconds;
        st->st_ctime = lookup_inode->status_seconds;
    }

    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    printf("Readdir\n");
    fflush(stdout);

    struct stat st;
    int rv;

    rv = nufs_getattr("/", &st);
    assert(rv == 0);

    // List all the files in the directory
    inode* root_inode = get_inode(ROOT_INODE);
    int dir_inum = tree_lookup(root_inode, path);
    assert(dir_inum >= 0 && dir_inum < INODE_COUNT);

    // Get the parent directory's inode
    inode* dir_inode = get_inode(dir_inum);
    int dir_pnum = dir_inode->pnum[0];
    dirent* dirents = (dirent*)pages_get_page(dir_pnum);
    int num_files = dir_inode->size / sizeof(dirent);

    for (int ii = 0; ii < num_files; ++ii) {
        // Get the path of the file
        dirent file_ent = dirents[ii];
        char* fname = file_ent.name;
        char fbuf[255];
        strcpy(fbuf, path);
        join_to_path(fbuf, fname);

        // Get the file status
        rv = nufs_getattr(fbuf, &st);
        assert(rv == 0);

        // Fill the buffer
        filler(buf, fname, &st, 0);
    }

    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

int
nufs_mksymlink(const char *path, mode_t mode) {

}


// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    printf("Mknod\n");
    fflush(stdout);

    int rv;

    struct timespec curr_time;
    rv = clock_gettime(CLOCK_REALTIME, &curr_time);

    // Get the name of the file
    char* path_cpy1 = strdup(path);
    char* path_cpy2 = strdup(path);
    char* file_name = basename(path_cpy1);
    char* parent_dir_name = dirname(path_cpy2);

    inode* root_inode = get_inode(ROOT_INODE);
    
    int parent_dir_inum = tree_lookup(root_inode, parent_dir_name);

    // Create a new inode and page for the new file
    int new_inum = alloc_inode();
    int new_pnum = alloc_page();

    if (new_inum < 0 || new_pnum < 0) {
        rv = -1;
    } else {
        // Init inode
        inode* new_inode = get_inode(new_inum);
        init_inode(new_inode, mode, new_pnum, 0, curr_time, 0);

        // Add the new file to the parent directory
        inode* parent_dir_inode = get_inode(parent_dir_inum);
        print_directory(parent_dir_inode);
        directory_put(parent_dir_inode, file_name, new_inum);
        print_directory(parent_dir_inode);
        rv = 0;
    }

    free(path_cpy1);
    free(path_cpy2);
    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir\n");
    fflush(stdout);

    int rv;

    // Get the name of the dir
    char* path_cpy = strdup(path);
    char* new_dir_name = basename(path_cpy);

    inode* root_inode = get_inode(ROOT_INODE);
    
    int dir_inum = get_parent_inum(root_inode, path);

    if (dir_inum < 0) {
        rv = dir_inum;
    } else {
        inode* dir_inode = get_inode(dir_inum);

        // Initialize the directory 
        int new_dir_inum = directory_init();

        // Put the directory in the directory it belongs to
        rv = directory_put(dir_inode, new_dir_name, new_dir_inum);
    }

    free(path_cpy);

    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_unlink(const char *path)
{
    printf("Unlink\n");
    fflush(stdout);

    // Get the file
    char* path_cpy = strdup(path);
    char* file_name = basename(path_cpy);
    
    inode* root_inode = get_inode(ROOT_INODE);
    int finum = tree_lookup(root_inode, path);

    inode* finode = get_inode(finum);

    // Decrease the ref count
    finode->refs -= 1;

    if (finode->refs == 0) {
        // Free the inode and free all the pages
        free_inode(finum);

        for (int ii = 0; ii < finode->pages_used; ++ii) {
            assert(finode->pnum[ii] > 0);
            free_page(finode->pnum[ii]);
        }
    }

    // Remove the file from the directory it belongs to
    int parent_dir_inum = get_parent_inum(root_inode, path);
    inode* parent_dir_inode = get_inode(parent_dir_inum);
    directory_delete(parent_dir_inode, file_name);

    free(path_cpy);

    int rv = 0;
    printf("unlink(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_link(const char *from, const char *to)
{
    printf("Link\n");
    fflush(stdout);

    int rv;

    // Get the inode being linked from
    inode* root_inode = get_inode(ROOT_INODE);
    int from_inum = tree_lookup(root_inode, from);
    inode* from_inode = get_inode(from_inum);

    char* to_cpy = strdup(to);
    char* to_file_name = basename(to_cpy);

    // Get the parent directory's inode
    int parent_dir_inum = get_parent_inum(root_inode, to);
    inode* parent_dir_inode = get_inode(parent_dir_inum);

    // Add the to file into the directory with a link to the from_inum
    rv = directory_put(parent_dir_inode, to_file_name, from_inum);

    // Increment references of from_inode
    from_inode->refs += 1;

    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{
    printf("Rmdir\n");
    fflush(stdout);

    int rv;

    if (streq(path, "/")) {
        rv = -1;
    } else {
        // Get the directory's inode
        inode* root_inode = get_inode(ROOT_INODE);
        int dir_inum = tree_lookup(root_inode, path);
        assert(dir_inum > 0);        

        inode* dir_inode = get_inode(dir_inum);

        // Free the directory pages
        int indirect_used = 0;
        for (int ii = 0; ii < dir_inode->pages_used; ++ii) {
            if (ii < 12) {
                free_page(dir_inode->pnum[ii]);
            } else {
                indirect_used = 1;
                break;
            }
        }

        // Handle indirect pointers
        if (indirect_used) {
            int* indirect_pages = (int*)pages_get_page(dir_inode->ind_pnum);
            for (int ii = 0; ii < dir_inode->pages_used - 12; ++ii) {
                free_page(indirect_pages[ii]);
            }
        }

        // Free the directory inode
        free_inode(dir_inum);

        // Remove the directory from the parent directory
        char* path_cpy = strdup(path);
        char* fname = basename(path_cpy);

        int parent_inum = get_parent_inum(root_inode, path);
        inode* parent_inode = get_inode(parent_inum);
        rv = directory_delete(parent_inode, fname);
    }

    
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
    printf("Rename\n");
    fflush(stdout);

    int rv;

    // Get the from and to names
    char* from_cpy = strdup(from);
    char* from_file_name = basename(from_cpy);

    char* to_cpy = strdup(to);
    char* to_file_name = basename(to_cpy);

    // Get the inode of the from file
    inode* root_inode = get_inode(ROOT_INODE);
    int from_inum = tree_lookup(root_inode, from);

    // Add the file to the new parent directory
    int to_parent_inum = get_parent_inum(root_inode, to);
    inode* to_parent_inode = get_inode(to_parent_inum);
    directory_put(to_parent_inode, to_file_name, from_inum);

    // Remove the file from the old parent directory
    int from_parent_inum = get_parent_inum(root_inode, from);
    inode* from_parent_inode = get_inode(from_parent_inum);
    rv = directory_delete(from_parent_inode, from_file_name);

    free(from_cpy);
    free(to_cpy);

    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
    printf("CHMOD\n");
    fflush(stdout);

    int rv = -1;
    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

void*
get_page_from_idx(inode* finode, int page_idx)
{
    assert(page_idx < finode->pages_used);

    void* data;
    if (page_idx < 12) {
        data = pages_get_page(finode->pnum[page_idx]);
    } else {
        // It is an indirect pointer
        int ind_pointer_idx = page_idx - 12;
        int ind_pnum = finode->ind_pnum;
        assert(ind_pnum >= 0);

        int* indirect_pointers = pages_get_page(finode->ind_pnum);
        int pnum = indirect_pointers[ind_pointer_idx];
        assert(ind_pnum >= 0);

        data = pages_get_page(pnum);
    }

    return data;
}

// Expand the file to size + offset bytes
int
truncate_expand(inode* finode, size_t size, off_t offset)
{
    long total_size = size + offset;
    int fsize = finode->size;

    int rv;

    if (fsize < total_size) {
        int num_pages_needed = bytes_to_pages(total_size);
        int last_page_idx = finode->pages_used - 1;

        if (fsize % 4096 != 0){
            int last_byte = fsize % 4096;
            int rest_of_last_page = 4096 - last_byte; 

            void* last_page = get_page_from_idx(finode, last_page_idx);
            void* start_point = (void*)((uintptr_t)last_page + last_byte);

            memset(start_point, 0, rest_of_last_page);
        }

        // Allocate and fill in the rest of the pages with 0
        ++last_page_idx;
        for (int ii = last_page_idx; ii < num_pages_needed; ++ii) {
            int page_num = alloc_page();
            assert(page_num != -1);
            finode->pages_used += 1;

            if (last_page_idx < 12) {
                // Allocated a page, pointed to by a direct pointer
                finode->pnum[ii] = page_num;
            } else {
                // Allocated a page, pointed to by an indirect pointer
                int ind_pointer_idx = ii - 12;
                int ind_pnum = finode->ind_pnum;
                if (ind_pnum == -1) {
                    // Allocate a page for indirect pointers
                    assert(ind_pointer_idx == 0);
                    ind_pnum = alloc_page();
                    finode->ind_pnum = ind_pnum;
                }

                int* ind_pointers = pages_get_page(ind_pnum);
                ind_pointers[ind_pointer_idx] = page_num;
            }
            
            // Fill the page up with NULL
            void* page = pages_get_page(page_num);
            memset(page, 0, 4096);
        }

        rv = 0;  
    } else {
        rv = -1;
    }

    return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
    printf("Truncate\n");
    fflush(stdout);

    int rv;

    if (size < 0) {
        rv = -EINVAL;
    } else {
        // Get the file inode       
        inode* root_inode = get_inode(ROOT_INODE);
        int finum = tree_lookup(root_inode, path);

        inode* finode = get_inode(finum);
        int fsize = finode->size;

        if (fsize < size) {
            // Expand the file from file size to size
            rv = truncate_expand(finode, size, 0);
            assert(rv >= 0);
            grow_inode(finode, size);
        } else if (fsize > size) {
            // Shorten the file from file size to size
            shrink_inode(finode, size);
        }

        rv = 0;
    }


    printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
    return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    printf("Open\n");
    fflush(stdout);

    int rv;

    struct timespec curr_time;
    rv = clock_gettime(CLOCK_REALTIME, &curr_time);
    assert(rv >= 0);

    // Upate the last viewed time for the file being opened
    inode* root_inode = get_inode(ROOT_INODE);
    int finum = tree_lookup(root_inode, path);
    inode* finode = get_inode(finum);
    finode->viewed_seconds = curr_time.tv_sec;

    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("Read(%s, %ld, %ld)\n", path, size, offset);
    fflush(stdout);

    int rv;

    // Get the inode for the file
    inode* root_inode = get_inode(ROOT_INODE);
    int finum = tree_lookup(root_inode, path);

    inode* finode = get_inode(finum);
    int fsize = finode->size;

    // Read bytes_to_read from the file
    int bytes_to_read = finode->size > size + offset ? size : finode->size - offset;
    int bytes_left_to_read = bytes_to_read;

    // Read from the initial offset
    int page_idx = offset / 4096;
    int initial_page_offset = offset % 4096;
    int bytes_from_curr_page = bytes_left_to_read + initial_page_offset > 4096 ? 4096 - initial_page_offset : bytes_left_to_read;

    void* data = get_page_from_idx(finode, page_idx);
    data = (void*)((uintptr_t)data + initial_page_offset);
    
    memcpy(buf, data, bytes_from_curr_page);

    bytes_left_to_read -= bytes_from_curr_page;
    ++page_idx;

    // Continue reading till all bytes have been read
    while (bytes_left_to_read > 0) {
        // Read from the next page
        bytes_from_curr_page = bytes_left_to_read > 4096 ? 4096 : bytes_left_to_read;
        data = get_page_from_idx(finode, page_idx);

        int bytes_read = bytes_to_read - bytes_left_to_read;
        void* dest_buf = (void*)((uintptr_t)buf + bytes_read);
        memcpy(dest_buf, data, bytes_from_curr_page);

        // Increment stuff
        bytes_left_to_read -= bytes_from_curr_page;
        ++page_idx;
    }

    printf("Bytes left to read: %d\n", bytes_left_to_read);
    assert(bytes_left_to_read == 0);

    // Null terminate the buffer?
    buf[bytes_to_read] = '\0';

    rv = bytes_to_read;

    printf("read(%s, %d bytes, @+%ld) -> %d\n", path, bytes_to_read, offset, rv);
    return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("Write\n");
    fflush(stdout);

    int rv;

    // Getting the time of the write
    struct timespec curr_time;
    rv = clock_gettime(CLOCK_REALTIME, &curr_time);
    assert(rv >= 0);
    
    // Getting the inode for the file
    inode* root_inode = get_inode(ROOT_INODE);
    int finum = tree_lookup(root_inode, path);

    inode* finode = get_inode(finum);
    int fsize = finode->size;

    if (size + offset > MAX_FILE_SIZE) {
        // Too large of a file
        int write_size = offset + size;
        printf("Attempting to write: %d\n", write_size);
        rv = -EFBIG;
    } else {
        // If space exists, carry on with the write

        // Expand the file to the given size + offset if needed
        long total_size = size + offset;
        if (finode->pages_used * 4096 < total_size) {
            rv = truncate_expand(finode, size, offset);
            assert(rv >= 0);
        }

        // First write at offset
        int bytes_left_to_write = size;
        int page_idx = offset / 4096;
        int initial_page_offset = offset % 4096;
        int bytes_to_curr_page = bytes_left_to_write + initial_page_offset > 4096 ? 4096 - initial_page_offset : bytes_left_to_write;

        char* data = get_page_from_idx(finode, page_idx);
        data += initial_page_offset;
        strncpy(data, buf, bytes_to_curr_page);

        bytes_left_to_write -= bytes_to_curr_page;
        ++page_idx;

        // Write the data page by page
        while (bytes_left_to_write > 0) {
            // Write current page
            bytes_to_curr_page = bytes_left_to_write > 4096 ? 4096 : bytes_left_to_write;
            data = get_page_from_idx(finode, page_idx);
            strncat(data, buf, bytes_to_curr_page);

            // Increment stuff
            bytes_left_to_write -= bytes_to_curr_page;
            ++page_idx;
        }

        finode->size = offset + size;
        rv = size;
    }
    
    // Upate the last written time
    finode->changed_seconds = curr_time.tv_sec;
    
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    printf("Utimens\n");
    fflush(stdout);

    int rv;

    // Get the inode for the file
    inode* root_inode = get_inode(ROOT_INODE);
    int inum = tree_lookup(root_inode, path);

    if (inum < 0) {
        rv = inum;
    } else {
        inode* lookup_inode = get_inode(inum);

        // Update the file's access, and modified time stamps
        lookup_inode->viewed_seconds = ts[0].tv_sec;
        lookup_inode->changed_seconds = ts[1].tv_sec;

        rv = 0;
    }

    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    printf("IOCTL\n");
    fflush(stdout);

    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

int nufs_symlink(const char *to, const char *from) {
  int symlink_mode = 0120000;
  
  printf("nufs_symlink\n");
  fflush(stdout);
  
  int rv;

  struct timespec curr_time;
  rv = clock_gettime(CLOCK_REALTIME, &curr_time);

  // Get the name of the file
  char* path_cpy1 = strdup(from);
  char* path_cpy2 = strdup(from);
  char* file_name = basename(path_cpy1);
  char* parent_dir_name = dirname(path_cpy2);

  inode* root_inode = get_inode(ROOT_INODE);
  
  int parent_dir_inum = tree_lookup(root_inode, parent_dir_name);

  // Create a new inode and page for the new file
  int new_inum = alloc_inode();
  int new_pnum = alloc_page();

  if (new_inum < 0 || new_pnum < 0) {
      rv = -1;
  } else {
      // Init inode
      inode* new_inode = get_inode(new_inum);
      init_inode(new_inode, symlink_mode, new_pnum, 0, curr_time, 1);

      // Add the new file to the parent directory
      inode* parent_dir_inode = get_inode(parent_dir_inum);
      print_directory(parent_dir_inode);
      directory_put(parent_dir_inode, file_name, new_inum);
      print_directory(parent_dir_inode);
      rv = 0;

      // add the to contents to the file.
      printf("%s of len %ld", to, strlen(to) + 1);
      rv = nufs_write(from, to, strlen(to)+1, 0, 0);
      assert(rv >= 0);
  }

  free(path_cpy1);
  free(path_cpy2);

  printf("nufs_symlink to %s from %s -> %i\n", to, from, rv);
  
  rv = (rv >= 0) ? 0 : rv;

  return rv;
}

int nufs_readlink(const char *path, char *buf, size_t size) {
  int rv = -1;

  rv = nufs_read(path, buf, size, 0, 0);

  rv = (rv >= 0) ? 0 : rv;
  
  printf("nufs_readlink contents: %s size: %lu -> %d\n", path, size, rv);
  return rv;
}


void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->link     = nufs_link;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
    ops->readlink = nufs_readlink;
    ops->symlink = nufs_symlink;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);

    // Creating the disk image and initializing it with root directory
    int ii = argc - 1;
    printf("Mount %s as a data file\n", argv[ii]);    
    char* path = malloc(strlen(argv[ii]) + 2);
    strcpy(path, "./");
    strcat(path, argv[--argc]);
    pages_init(path);

    free(path);

    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}
