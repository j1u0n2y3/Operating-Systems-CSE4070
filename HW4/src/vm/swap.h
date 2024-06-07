#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>
#include <bitmap.h>

struct bitmap *swap_bitmap;

void swap_init(void);
void swap_in(size_t index, void *kaddr);
size_t swap_out(void *kaddr);
void swap_free(size_t index);

#endif
