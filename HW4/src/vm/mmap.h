#ifndef VM_MMAP_H
#define VM_MMAP_H

#include <list.h>
#include "filesys/file.h"

struct mm_entry
{
  unsigned int mapid;
  struct file *file;
  struct list pte_list;
  struct list_elem elem;
};

unsigned int mm_map(int fd, void *addr);
void mm_free(unsigned int mapid);

#endif
