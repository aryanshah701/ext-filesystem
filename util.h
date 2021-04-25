// based on cs3650 starter code

#ifndef UTIL_H
#define UTIL_H

#include <string.h>

static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}

static int
min(int x, int y)
{
    return (x < y) ? x : y;
}

static int
max(int x, int y)
{
    return (x > y) ? x : y;
}

static int
clamp(int x, int v0, int v1)
{
    return max(v0, min(x, v1));
}

static int
bytes_to_pages(int bytes)
{
    int quo = bytes / 4096;
    int rem = bytes % 4096;
    if (rem == 0) {
        return quo;
    }
    else {
        return quo + 1;
    }
}

static void
join_to_path(char* buf, char* item)
{
    int nn = strlen(buf);
    if (buf[nn - 1] != '/') {
        strcat(buf, "/");
    }
    strcat(buf, item);
}

static int
items_per_page(int item_size) 
{
    int quo = 4096 / item_size;
    int rem = 4096 % item_size;
    if (rem == 0) {
        return quo;
    }
    else {
        return quo + 1;
    }
}

static int
pages_used(int item_size, int total_size)
{
    int items_per_page_ = items_per_page(item_size);
    int num_pages = total_size / items_per_page_;
    int quo = total_size * items_per_page_;
    int rem = total_size % items_per_page_;
    if (rem == 0) {
        return quo;
    } else {
        return quo + 1;
    }
}



#endif
