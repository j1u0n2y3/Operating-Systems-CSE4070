#include "vm/frame.h"
#include "vm/swap.h"
#include "lib/string.h"
#include "threads/malloc.h"

static void ft_insert(struct frame *frame);
static void ft_delete(struct frame *frame);
static struct frame *ft_find(void *kaddr);
static struct list_elem *ft_clock(void);
static struct frame *ft_get(void);
static void ft_evict(void);

struct lock frame_lock;

void frame_init(void)
{
  lock_init(&frame_lock);
  list_init(&frame_list);
  victim = NULL;
  return;
}

struct frame *alloc_page(enum palloc_flags flags)
{
  struct frame *page = (struct frame *)calloc(1, sizeof(struct frame));
  if (!page)
    return page;

  page->thread = thread_current();
  page->kaddr = palloc_get_page(flags);

  for (;;)
  {
    if (page->kaddr != NULL)
      break;

    lock_acquire(&frame_lock);
    ft_evict();
    lock_release(&frame_lock);
    page->kaddr = palloc_get_page(flags);
  }

  lock_acquire(&frame_lock);
  ft_insert(page);
  lock_release(&frame_lock);

  return page;
}

void free_page(void *kaddr)
{
  lock_acquire(&frame_lock);

  struct frame *page = ft_find(kaddr);
  if (!page)
  {
    lock_release(&frame_lock);
    return;
  }

  ft_delete(page);
  pagedir_clear_page(page->thread->pagedir, page->pte->vaddr);
  palloc_free_page(page->kaddr);
  free(page);

  lock_release(&frame_lock);
  return;
}

bool load_file_to_page(void *kaddr, struct pt_entry *pte)
{
  size_t read_byte = pte->read_bytes;
  size_t tmp = (size_t)file_read_at(pte->file, kaddr, pte->read_bytes, pte->offset);

  if (read_byte != tmp)
    return false;

  memset(kaddr + pte->read_bytes, 0, pte->zero_bytes);

  return true;
}

static void ft_insert(struct frame *frame)
{
  list_push_back(&frame_list, &(frame->frame_elem));
  return;
}

static void ft_delete(struct frame *frame)
{
  struct list_elem *entry = &(frame->frame_elem), *ret = list_remove(entry);
  victim = (victim == entry) ? ret : victim;
  return;
}

static struct frame *ft_find(void *kaddr)
{
  for (struct list_elem *iter = list_begin(&frame_list);
       iter != list_end(&frame_list);
       iter = list_next(iter))
  {
    struct frame *entry = list_entry(iter, struct frame, frame_elem);

    if (entry->kaddr == kaddr)
      return entry;
  }
  return NULL;
}

static struct list_elem *ft_clock(void)
{
  if (victim != NULL && victim != list_end(&frame_list))
  {
    victim = list_next(victim);

    if (victim != list_end(&frame_list))
      return victim;
  }

  if (list_empty(&frame_list))
    return victim;

  victim = list_begin(&frame_list);
  return victim;
}

static struct frame *ft_get(void)
{
  for (;;)
  {
    struct frame *entry = list_entry(ft_clock(), struct frame, frame_elem);

    if (!pagedir_is_accessed(entry->thread->pagedir, entry->pte->vaddr))
      return entry;

    pagedir_set_accessed(entry->thread->pagedir, entry->pte->vaddr, 0);
  }
  return NULL;
}

static void ft_evict(void)
{
  struct frame *frame = ft_get();
  bool is_dirty = pagedir_is_dirty(frame->thread->pagedir, frame->pte->vaddr);
  pt_type type = frame->pte->type;

  if (is_dirty && type == MAPPED)
    file_write_at(frame->pte->file, frame->kaddr, frame->pte->read_bytes, frame->pte->offset);

  else if (is_dirty && type == BINARY)
  {
    frame->pte->swap_slot = swap_out(frame->kaddr);
    frame->pte->type = SWAPPED;
  }

  else if (type == SWAPPED)
    frame->pte->swap_slot = swap_out(frame->kaddr);

  frame->pte->is_loaded = false;
  ft_delete(frame);
  pagedir_clear_page(frame->thread->pagedir, frame->pte->vaddr);
  palloc_free_page(frame->kaddr);
  free(frame);
  return;
}
