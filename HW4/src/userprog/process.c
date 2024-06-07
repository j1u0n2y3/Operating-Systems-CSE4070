#include "userprog/process.h"
#include "userprog/syscall.h"
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/mmap.h"

static thread_func start_process NO_RETURN;
static bool load(const char *cmdline, void (**eip)(void), void **esp);
static int parsing(char *buf, char **argv);

extern struct lock file_lock;

int process_execute(const char *file_name)
{
  char *fn_copy = palloc_get_page(0);
  if (!fn_copy)
    return TID_ERROR;

  strlcpy(fn_copy, file_name, PGSIZE);

  int i = 0;
  while (file_name[i] == ' ')
    i++;

  char temp_name[128];
  int idx = 0;
  int cmd_len = strlen(file_name) + 1;
  while (i < cmd_len && file_name[i] != ' ' && file_name[i] != '\0')
  {
    temp_name[idx++] = file_name[i++];
  }
  temp_name[idx] = '\0';

  int tid = thread_create(temp_name, PRI_DEFAULT, start_process, fn_copy);

  sema_down(&(thread_current()->thread_lock));

  if (tid == TID_ERROR)
  {
    palloc_free_page(fn_copy);
    return tid;
  }

  struct list *cur_list = &(thread_current()->child_list);
  for (struct list_elem *iter = list_begin(cur_list);
       iter != list_end(cur_list);
       iter = list_next(iter))
  {
    struct thread *entry = list_entry(iter, struct thread, child_elem);
    if (entry->exit_status == -1)
      return process_wait(tid);
  }

  return tid;
}

static void
start_process(void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool res;

  pt_init(&(thread_current()->pt));

  memset(&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  res = load(file_name, &if_.eip, &if_.esp);

  sema_up(&((thread_current()->parent)->thread_lock));

  palloc_free_page(file_name);
  if (!res)
    exit(-1);

  asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(&if_) : "memory");
  NOT_REACHED();
}

int process_wait(int child_tid UNUSED)
{
  struct list *cur_list = &(thread_current()->child_list);

  for (struct list_elem *iter = list_begin(cur_list);
       iter != list_end(cur_list);
       iter = list_next(iter))
  {
    struct thread *entry = list_entry(iter, struct thread, child_elem);
    if (entry->tid == child_tid)
    {
      sema_down(&(entry->parent_lock));
      list_remove(&(entry->child_elem));
      sema_up(&(entry->child_lock));
      return (entry->exit_status);
    }
  }

  return -1;
}

void process_exit(void)
{
  struct thread *cur = thread_current();
  for (unsigned int i = 1; i < cur->map_list_size; i++)
    munmap(i);

  file_close(cur->file);
  pt_destroy(&(cur->pt));

  uint32_t *pd = cur->pagedir;
  if (pd != NULL)
  {
    cur->pagedir = NULL;
    pagedir_activate(NULL);
    pagedir_destroy(pd);
  }

  sema_up(&(cur->parent_lock));
  sema_down(&(cur->child_lock));
  return;
}

void process_activate(void)
{
  struct thread *t = thread_current();

  pagedir_activate(t->pagedir);

  tss_update();
}

typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32 /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32 /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32 /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16 /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
{
  unsigned char e_ident[16];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
};

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
};

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL 0           /* Ignore. */
#define PT_LOAD 1           /* Loadable segment. */
#define PT_DYNAMIC 2        /* Dynamic linking info. */
#define PT_INTERP 3         /* Name of dynamic loader. */
#define PT_NOTE 4           /* Auxiliary info. */
#define PT_SHLIB 5          /* Reserved. */
#define PT_PHDR 6           /* Program header table. */
#define PT_STACK 0x6474e551 /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

static bool setup_stack(void **esp);
static bool validate_segment(const struct Elf32_Phdr *, struct file *);
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool is_writable);

bool load(const char *file_name, void (**eip)(void), void **esp)
{
  struct Elf32_Ehdr ehdr;
  struct thread *t = thread_current();
  struct file *file = NULL;
  off_t file_ofs;
  char *file_name_copy = file_name;
  char *argv[128];
  int argc, i;
  bool res = false;

  t->pagedir = pagedir_create();
  if (!(t->pagedir))
    goto done;
  process_activate();

  argc = parsing(file_name_copy, argv);
  strlcpy(thread_name(), argv[0], strlen(argv[0]) + 1);

  lock_acquire(&file_lock);

  file = filesys_open(file_name);
  if (file == NULL)
  {
    lock_release(&file_lock);
    printf("load: %s: open failed\n", file_name);
    goto done;
  }
  t->file = file;

  lock_release(&file_lock);

  if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr || memcmp(ehdr.e_ident, "\177ELF\1\1\1", 7) || ehdr.e_type != 2 || ehdr.e_machine != 3 || ehdr.e_version != 1 || ehdr.e_phentsize != sizeof(struct Elf32_Phdr) || ehdr.e_phnum > 1024)
  {
    printf("load: %s: error loading executable\n", file_name);
    goto done;
  }

  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
  {
    struct Elf32_Phdr phdr;

    if (file_ofs < 0 || file_ofs > file_length(file))
      goto done;
    file_seek(file, file_ofs);

    if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
      goto done;
    file_ofs += sizeof phdr;
    switch (phdr.p_type)
    {
    case PT_NULL:
    case PT_NOTE:
    case PT_PHDR:
    case PT_STACK:
    default:
      break;
    case PT_DYNAMIC:
    case PT_INTERP:
    case PT_SHLIB:
      goto done;
    case PT_LOAD:
      if (validate_segment(&phdr, file))
      {
        bool is_writable = (phdr.p_flags & PF_W) != 0;
        uint32_t file_page = phdr.p_offset & ~PGMASK;
        uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
        uint32_t page_offset = phdr.p_vaddr & PGMASK;
        uint32_t read_bytes, zero_bytes;
        if (phdr.p_filesz > 0)
        {
          read_bytes = page_offset + phdr.p_filesz;
          zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) - read_bytes);
        }
        else
        {
          read_bytes = 0;
          zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
        }
        if (!load_segment(file, file_page, (void *)mem_page,
                          read_bytes, zero_bytes, is_writable))
          goto done;
      }
      else
        goto done;
      break;
    }
  }

  if (!setup_stack(esp))
    goto done;

  size_t total_len = 0, temp_len;
  for (i = argc - 1; i >= 0; i--)
  {
    temp_len = (strlen(argv[i]) + 1);
    total_len += temp_len;

    *esp = (uint8_t *)(*esp) - temp_len;
    memcpy(*esp, argv[i], temp_len);
    argv[i] = *esp;
  }

  temp_len = 4 - (total_len % 4);
  if (temp_len != 4)
  {
    while (temp_len--)
    {
      *esp = (uint8_t *)(*esp) - 1;
      memset(*esp, 0, 1);
    }
  }

  for (i = argc; i >= 0; i--)
  {
    *esp = (uint8_t *)(*esp) - 4;
    if (i == argc)
      *(uint32_t *)(*esp) = 0;
    else
      *(uint32_t *)(*esp) = (uint32_t)argv[i];
  }
  *esp = (uint8_t *)(*esp) - 4;
  *(uint32_t *)(*esp) = (uint32_t)((uint8_t *)(*esp) + 4);
  *esp = (uint8_t *)(*esp) - 4;
  *(uint32_t *)(*esp) = argc;
  *esp = (uint8_t *)(*esp) - 4;
  *(uint32_t *)(*esp) = 0;

  *eip = (void (*)(void))ehdr.e_entry;

  res = true;

done:
  return res;
}

static bool install_page(void *upage, void *kpage, bool is_writable);

static bool
validate_segment(const struct Elf32_Phdr *phdr, struct file *file)
{
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  if (phdr->p_offset > (Elf32_Off)file_length(file))
    return false;

  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  if (phdr->p_memsz == 0)
    return false;

  if (!is_user_vaddr((void *)phdr->p_vaddr))
    return false;
  if (!is_user_vaddr((void *)(phdr->p_vaddr + phdr->p_memsz)))
    return false;

  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  if (phdr->p_vaddr < PGSIZE)
    return false;

  return true;
}

static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes, bool is_writable)
{
  ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT(pg_ofs(upage) == 0);
  ASSERT(ofs % PGSIZE == 0);

  file_seek(file, ofs);
  for (/***/;
       read_bytes > 0 || zero_bytes > 0;
       upage += PGSIZE)
  {
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct pt_entry *pte = pt_create(upage, BINARY, is_writable, false,
                                     file, ofs, page_read_bytes, page_zero_bytes);
    if (!pte)
      return false;

    pt_insert(&(thread_current()->pt), pte);

    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    ofs += page_read_bytes;
  }
  return true;
}

static bool setup_stack(void **esp)
{
  struct frame *kpage = alloc_page(PAL_USER | PAL_ZERO);
  if (!kpage)
    return false;

  uint8_t *base_addr = ((uint8_t *)PHYS_BASE) - PGSIZE;
  bool res = install_page(base_addr, kpage->kaddr, true);

  if (res)
  {
    *esp = PHYS_BASE;
    struct pt_entry *new_pte = pt_create(base_addr, SWAPPED, true, true, NULL, 0, 0, 0);
    if (!new_pte)
      return false;
    kpage->pte = new_pte;
    pt_insert(&(thread_current()->pt), new_pte);
  }
  else
    free_page(kpage->kaddr);

  return res;
}

static bool
install_page(void *upage, void *kpage, bool is_writable)
{
  struct thread *t = thread_current();

  return (pagedir_get_page(t->pagedir, upage) == NULL && pagedir_set_page(t->pagedir, upage, kpage, is_writable));
}

static int parsing(char *buf, char **argv)
{
  int argc = 0;
  char *tmp;

  buf[strlen(buf) - (buf[strlen(buf) - 1] == '\n')] = ' ';
  for (; *buf == ' '; ++buf)
    ;

  while ((tmp = strchr(buf, ' ')))
  {
    argv[argc++] = buf;
    *tmp = '\0';
    for (buf = tmp + 1; *buf == ' '; ++buf)
      ;
  }
  argv[argc] = NULL;
  return argc;
}

bool expand_stack(void *addr, void *esp)
{
  if (!is_user_vaddr(addr) ||
      addr < (PHYS_BASE - 0x8000000 /* MAX STACK SIZE */) ||
      addr < (esp - 32))
    return false;

  void *upage = pg_round_down(addr);
  struct frame *kpage = alloc_page(PAL_USER | PAL_ZERO);
  bool res = kpage && install_page(upage, kpage->kaddr, true);

  if (res)
  {
    kpage->pte = pt_create(upage, SWAPPED, true, true, NULL, 0, 0, 0);
    res = kpage->pte != NULL;
    if (res)
      pt_insert(&(thread_current()->pt), kpage->pte);
    else
      free_page(kpage->kaddr);
  }
  else if (kpage)
    free_page(kpage->kaddr);

  return res;
}

bool mm_fault_handler(struct pt_entry *pte)
{
  struct frame *kpage = alloc_page(PAL_USER);
  kpage->pte = pte;
  bool is_binary_or_mapped = (pte->type == BINARY || pte->type == MAPPED);
  bool is_swapped = (pte->type == SWAPPED);

  bool res = false;
  if (is_binary_or_mapped && load_file_to_page(kpage->kaddr, pte))
    res = install_page(pte->vaddr, kpage->kaddr, pte->is_writable);
  else if (is_swapped)
  {
    swap_in(pte->swap_slot, kpage->kaddr);
    res = install_page(pte->vaddr, kpage->kaddr, pte->is_writable);
  }

  if (!res)
    free_page(kpage->kaddr);
  else
    pte->is_loaded = true;

  return res;
}
