// based on cs3650 starter code

#ifndef BITMAP_H
#define BITMAP_H

int bitmap_get(void* bm, int ii);
void bitmap_put(void* bm, int ii, int vv);
void bitmap_print(void* bm, int size);
void bitmap_init(void* bm, int size);
int bitmap_num_zeros(void* bm, int size);

#endif
