#include "vm/mmap.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "filesys/off_t.h"

extern struct lock file_lock;
static struct file *mm_get_file(int fd);

static struct mm_entry *mm_get_entry(unsigned int mapid);

unsigned int mm_map(int fd, void *addr)
{
  if (pg_ofs(addr) || !addr || !is_user_vaddr(addr) || pt_find(addr))
    return (unsigned int)-1;

  struct mm_entry *mme = (struct mm_entry *)malloc(sizeof(struct mm_entry));
  if (!mme)
    return (unsigned int)-1;

  lock_acquire(&file_lock);
  mme->file = file_reopen(mm_get_file(fd));
  lock_release(&file_lock);

  mme->mapid = (thread_current()->map_list_size)++;
  list_init(&mme->pte_list);
  list_push_back(&(thread_current()->map_list), &(mme->elem));

  size_t ofs = 0;
  for (off_t file_len = file_length(mme->file);
       file_len > 0;
       addr += PGSIZE, ofs += PGSIZE, file_len -= PGSIZE)
  {
    size_t page_read_bytes = (file_len < PGSIZE) ? file_len : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct pt_entry *pte = pt_create(addr, MAPPED, true, false, mme->file, ofs, page_read_bytes, page_zero_bytes);
    if (!pte)
      return false;

    pt_insert(&(thread_current()->pt), pte);
    list_push_back(&(mme->pte_list), &(pte->mm_elem));
  }

  return mme->mapid;
}

void mm_free(unsigned int mapid)
{
  struct mm_entry *mme = mm_get_entry(mapid);
  if (!mme)
    return;

  for (struct list_elem *entry = list_begin(&(mme->pte_list));
       entry != list_end(&(mme->pte_list));
       /***/)
  {
    struct pt_entry *pte = list_entry(entry, struct pt_entry, mm_elem);

    if (pte->is_loaded && pagedir_is_dirty(thread_current()->pagedir, pte->vaddr))
    {
      lock_acquire(&file_lock);

      size_t read_byte = pte->read_bytes;
      size_t temp = (size_t)file_write_at(pte->file, pte->vaddr, pte->read_bytes, pte->offset);

      if (read_byte != temp)
        NOT_REACHED();

      lock_release(&file_lock);

      free_page(pagedir_get_page(thread_current()->pagedir, pte->vaddr));
    }

    pte->is_loaded = false;
    entry = list_remove(entry);
    pt_delete(&(thread_current()->pt), pte);
  }

  list_remove(&(mme->elem));
  free(mme);
  return;
}

static struct file *mm_get_file(int fd)
{
  struct file *res = NULL;
  if (3 <= fd && fd < 128)
    res = thread_current()->fd[fd];
  return res;
}

static struct mm_entry *mm_get_entry(unsigned int mapid)
{
  for (struct list_elem *entry = list_begin(&(thread_current()->map_list));
       entry != list_end(&(thread_current()->map_list));
       entry = list_next(entry))
  {
    struct mm_entry *entry_element = list_entry(entry, struct mm_entry, elem);
    if (entry_element->mapid == mapid)
      return entry_element;
  }
  return NULL;
}
