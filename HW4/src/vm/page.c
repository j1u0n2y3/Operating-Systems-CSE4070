#include <string.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

static unsigned hash_func(const struct hash_elem *h_elem, void *UNUSED);
static bool comp_func(const struct hash_elem *left, const struct hash_elem *right, void *UNUSED);
static void destroy_func(struct hash_elem *h_elem, void *UNUSED);

void pt_init(struct hash *pt)
{
  hash_init(pt, hash_func, comp_func, NULL);
  return;
}

void pt_destroy(struct hash *pt)
{
  hash_destroy(pt, destroy_func);
  return;
}

struct pt_entry *pt_create(void *vaddr, pt_type type, bool is_writable, bool is_loaded,
                           struct file *file, size_t offset, size_t read_bytes, size_t zero_bytes)
{
  struct pt_entry *pte = (struct pt_entry *)calloc(1, sizeof(struct pt_entry));
  if (!pte)
    return pte;

  pte->vaddr = vaddr;
  pte->type = type;
  pte->is_writable = is_writable;
  pte->is_loaded = is_loaded;
  pte->file = file;
  pte->offset = offset;
  pte->read_bytes = read_bytes;
  pte->zero_bytes = zero_bytes;

  return pte;
}

bool pt_insert(struct hash *pt, struct pt_entry *pte)
{
  return hash_insert(pt, &(pte->elem)) == NULL;
}

bool pt_delete(struct hash *pt, struct pt_entry *pte)
{
  bool deleted = hash_delete(pt, &(pte->elem)) != NULL;
  if (deleted)
  {
    free_page(pagedir_get_page(thread_current()->pagedir, pte->vaddr));
    swap_free(pte->swap_slot);
    free(pte);
  }
  return deleted;
}

struct pt_entry *pt_find(void *vaddr)
{
  struct pt_entry tmp = {.vaddr = pg_round_down(vaddr)};
  struct hash_elem *entry = hash_find(&(thread_current()->pt), &(tmp.elem));
  return (entry ? hash_entry(entry, struct pt_entry, elem) : NULL);
}

static unsigned hash_func(const struct hash_elem *h_elem, void *aux UNUSED)
{
  return hash_int((int)(hash_entry(h_elem, struct pt_entry, elem)->vaddr));
}

static bool comp_func(const struct hash_elem *left, const struct hash_elem *right, void *aux UNUSED)
{
  return (hash_entry(left, struct pt_entry, elem)->vaddr <
          hash_entry(right, struct pt_entry, elem)->vaddr);
}

static void destroy_func(struct hash_elem *h_elem, void *aux UNUSED)
{
  struct pt_entry *pte = hash_entry(h_elem, struct pt_entry, elem);
  free_page(pagedir_get_page(thread_current()->pagedir, pte->vaddr));
  swap_free(pte->swap_slot);

  free(pte);
  return;
}
