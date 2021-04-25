#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "bitmap.h"

int 
bitmap_get(void* bm, int ii) 
{
    char* char_arr = (char*)bm;
    if (char_arr[ii] == '0') {
        return 0;
    } else if (char_arr[ii] == '1') {
        return 1;
    } else {
        fprintf(stderr, "invalid value in bitmap: %c\n", char_arr[ii]);
        abort();
    }
}

void 
bitmap_put(void* bm, int ii, int vv) 
{
    char* char_arr = (char*)bm;
    if (vv == 0) {
        char_arr[ii] = '0';
    } else if (vv == 1) {
        char_arr[ii] = '1';
    } else {
        fprintf(stderr, "invalid vv to put in bitmap: %d\n", vv);
        abort();
    }
}

void 
bitmap_print(void* bm, int size) 
{
    char* char_arr = (char*)bm;
    printf("Printing Bitmap\n");
    for (int ii = 0; ii < size; ++ii) {
        printf("%d: %c\n", ii, char_arr[ii]);
    }
    printf("End of printing Bitmap\n");
}

void
bitmap_init(void* bm, int size)
{
    char* char_arr = (char*)bm;
    for (int ii = 0; ii < size; ++ii) {
        char_arr[ii] = '0';
    }
}

int
bitmap_num_zeros(void* pbm, int size)
{
    char* char_arr = (char*)pbm;
    int num_zeros = 0;
    for (int ii = 0; ii < size; ++ii) {
        assert(char_arr[ii] == '0' || char_arr[ii] == '1');
        if (char_arr[ii] == '0') {
            num_zeros += 1;
        }
    }

    return num_zeros;

}
