#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/thread.h"

struct frame
{
  void *kaddr;
  struct thread *thread;
  struct pt_entry *pte;
  struct list_elem frame_elem;
};

struct list frame_list;
struct list_elem *victim;

void frame_init(void);
struct frame *alloc_page(enum palloc_flags flags);
void free_page(void *kaddr);
bool load_file_to_page(void *kaddr, struct pt_entry *pte);

#endif
