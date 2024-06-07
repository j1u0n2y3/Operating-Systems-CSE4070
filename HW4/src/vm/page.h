#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"

typedef enum
{
  BINARY,
  MAPPED,
  SWAPPED
} pt_type;

struct pt_entry
{
  void *vaddr;
  pt_type type;
  bool is_writable;
  bool is_loaded;
  struct file *file;
  size_t offset;
  size_t read_bytes;
  size_t zero_bytes;

  struct hash_elem elem;
  struct list_elem mm_elem;

  size_t swap_slot;
};

void pt_init(struct hash *pt);
void pt_destroy(struct hash *pt);
struct pt_entry *pt_create(void *vaddr, pt_type type, bool is_writable, bool is_loaded, struct file *file, size_t offset, size_t read_bytes, size_t zero_bytes);
bool pt_insert(struct hash *pt, struct pt_entry *pte);
bool pt_delete(struct hash *pt, struct pt_entry *pte);
struct pt_entry *pt_find(void *vaddr);

#endif
