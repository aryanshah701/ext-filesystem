// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#include "pages.h"
#include "util.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"

const int PAGE_COUNT = 256;
const int NUFS_SIZE  = 4096 * 256; // 1MB

static int   pages_fd   = -1;
static void* pages_base =  0;

void
pages_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0644);
    assert(pages_fd >= 0 || errno == EEXIST);
    
    if (pages_fd >= 0) {
        // If an image doesn't exist create one
        int rv = ftruncate(pages_fd, NUFS_SIZE);
        assert(rv == 0);

        pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
        assert(pages_base != MAP_FAILED);

        // Init the pages and inode bitmaps
        void* pbm = get_pages_bitmap();
        bitmap_init(pbm, PAGE_COUNT);
        bitmap_put(pbm, 0, 1);
        bitmap_put(pbm, 1, 1);
        bitmap_put(pbm, 2, 1);
        bitmap_put(pbm, 3, 1);

        void* ibm = get_inode_bitmap();
        bitmap_init(ibm, INODE_COUNT);

        // Initialising the root directory(should be inode 0)
        int root_inum = directory_init();
        assert(root_inum == 0);
    } else {
        // If an image does exist, simply use that
        pages_fd = open(path, O_RDWR, 0644);
        assert(pages_fd != -1);

        pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
        assert(pages_base != MAP_FAILED);
    }
}

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}

void*
get_pages_bitmap()
{
    return pages_get_page(0);
}

void*
get_inode_bitmap()
{
    uint8_t* page = pages_get_page(0);
    return (void*)(page + PAGE_COUNT);
}

int
alloc_page()
{
    void* pbm = get_pages_bitmap();

    for (int ii = 1; ii < PAGE_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii)) {
            bitmap_put(pbm, ii, 1);
            printf("+ alloc_page() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void
free_page(int pnum)
{
    printf("+ free_page(%d)\n", pnum);
    void* pbm = get_pages_bitmap();
    bitmap_put(pbm, pnum, 0);
}

int
pages_get_pages_free()
{
    printf("get_num_pages_free");
    void* pbm = get_pages_bitmap();
    int num_pages_free = bitmap_num_zeros(pbm, PAGE_COUNT);
    return num_pages_free;
}