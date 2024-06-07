#include "userprog/syscall.h"
#include "userprog/process.h"
#include <syscall-nr.h>
#include <stdio.h>
#include <string.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "vm/mmap.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  lock_init(&file_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *esp = f->esp;
  uint32_t sys_num = *esp;

  switch (sys_num)
  {
  case SYS_HALT:
    shutdown_power_off();
    break;

  case SYS_EXIT:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    exit(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_EXEC:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = exec(*(char **)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_WAIT:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = wait(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_CREATE:
    for (int i = 1; i <= 2; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = create(*(char **)((uint8_t *)esp + 4 * 1),
                    *(unsigned *)((uint8_t *)esp + 4 * 2));
    break;

  case SYS_REMOVE:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = remove(*(char **)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_OPEN:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = open(*(char **)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_FILESIZE:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = filesize(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_READ:
    for (int i = 1; i <= 3; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = read(*(int *)((uint8_t *)esp + 4 * 1), *(void **)((uint8_t *)esp + 4 * 2), *(unsigned *)((uint8_t *)esp + 4 * 3));
    break;

  case SYS_WRITE:
    for (int i = 1; i <= 3; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = write(*(int *)((uint8_t *)esp + 4 * 1), *(void **)((uint8_t *)esp + 4 * 2), *(unsigned *)((uint8_t *)esp + 4 * 3));
    break;

  case SYS_SEEK:
    for (int i = 1; i <= 2; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    seek(*(int *)((uint8_t *)esp + 4 * 1), *(unsigned *)((uint8_t *)esp + 4 * 2));
    break;

  case SYS_TELL:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = tell(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_CLOSE:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    close(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_FIBONACCI:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = fibonacci(*(int *)((uint8_t *)esp + 4 * 1));
    break;

  case SYS_MAX_OF_FOUR_INT:
    for (int i = 1; i <= 4; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = max_of_four_int(*(int *)((uint8_t *)esp + 4 * 1), *(int *)((uint8_t *)esp + 4 * 2), *(int *)((uint8_t *)esp + 4 * 3), *(int *)((uint8_t *)esp + 4 * 4));
    break;

  case SYS_MMAP:
    for (int i = 1; i <= 2; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    f->eax = mmap(*(int *)((uint8_t *)esp + 4 * 1),
                  *(void **)((uint8_t *)esp + 4 * 2));
    break;

  case SYS_MUNMAP:
    for (int i = 1; i <= 1; i++)
    {
      uint8_t *arg_addr = ((uint8_t *)esp + 4 * i);
      if (arg_addr == NULL || is_user_vaddr(arg_addr) == false)
        exit(-1);
      if (!pt_find(arg_addr))
      {
        if (!expand_stack(arg_addr, esp))
          exit(-1);
      }
    }
    munmap(*(unsigned int *)((uint8_t *)esp + 4 * 1));
    break;

  default:
    break;
  }
}

void exit(int status)
{
  struct file **f_list = &(thread_current()->fd);
  struct list *c_list = &(thread_current()->child_list);
  struct list_elem *iter;
  int i;

  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;

  for (i = 0; i < 128; i++)
  {
    if (*(f_list + i))
      close(i);
  }

  iter = list_begin(c_list);
  while (iter != list_end(c_list))
  {
    wait(list_entry(iter, struct thread, child_elem)->tid);
    iter = list_next(iter);
  }

  thread_exit();
  return;
}

int exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

int wait(int pid)
{
  return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{
  if (file == NULL || is_user_vaddr(file) == false)
    exit(-1);

  lock_acquire(&file_lock);
  bool res = filesys_create(file, initial_size);
  lock_release(&file_lock);

  return res;
}

bool remove(const char *file)
{
  if (file == NULL || is_user_vaddr(file) == false)
    exit(-1);

  lock_acquire(&file_lock);
  bool res = filesys_remove(file);
  lock_release(&file_lock);

  return res;
}

int open(const char *file)
{
  if (file == NULL || is_user_vaddr(file) == false)
    exit(-1);

  lock_acquire(&file_lock);

  int idx = -1;

  struct file *f = filesys_open(file);
  if (!f)
  {
    lock_release(&file_lock);
    return idx;
  }

  if (!strcmp(thread_name(), file))
    file_deny_write(f);

  struct file **fd_list = &(thread_current()->fd);
  for (int i = 3; i < 128; i++)
  {
    if (fd_list[i] == NULL)
    {
      idx = i;
      fd_list[i] = f;
      break;
    }
  }

  lock_release(&file_lock);

  return idx;
}

int filesize(int fd)
{
  if (fd < 3 || fd >= 128)
    exit(-1);

  lock_acquire(&file_lock);

  struct file *f = thread_current()->fd[fd];
  if (!f)
  {
    lock_release(&file_lock);
    exit(-1);
  }
  int size = file_length(f);

  lock_release(&file_lock);

  return size;
}

int read(int fd, void *buffer, unsigned size)
{
  if (!buffer || !is_user_vaddr(buffer))
    exit(-1);

  lock_acquire(&file_lock);

  struct file *f = (fd >= 3 && fd < 128) ? thread_current()->fd[fd] : NULL;
  off_t bytes = 0;

  if (fd == 0)
  {
    char *buf_ptr = (char *)buffer;
    for (unsigned i = 0; i < size; ++i)
    {
      char c = input_getc();
      if (c == '\0')
        break;
      buf_ptr[i] = c;
      bytes++;
    }
    buf_ptr[bytes] = '\0';
  }
  else if (!f)
  {
    lock_release(&file_lock);
    exit(-1);
  }
  else
    bytes = file_read(f, buffer, size);

  lock_release(&file_lock);
  return bytes;
}

int write(int fd, const void *buffer, unsigned size)
{
  if (!buffer || !is_user_vaddr(buffer))
    exit(-1);

  lock_acquire(&file_lock);

  off_t bytes = 0;
  struct file *f = (fd > 1 && fd < 128) ? thread_current()->fd[fd] : NULL;

  if (fd == 1)
  {
    putbuf(buffer, size);
    bytes = size;
  }
  else if (!f)
  {
    lock_release(&file_lock);
    exit(-1);
  }
  else
  {
    if (f->deny_write)
      file_deny_write(f);

    bytes = file_write(f, buffer, size);
  }

  lock_release(&file_lock);
  return bytes;
}

void seek(int fd, unsigned position)
{
  if (fd < 3 || fd >= 128)
    exit(-1);

  lock_acquire(&file_lock);

  struct file *f = thread_current()->fd[fd];
  if (!f)
  {
    lock_release(&file_lock);
    exit(-1);
  }

  file_seek(f, position);

  lock_release(&file_lock);
  return;
}

unsigned int tell(int fd)
{
  if (fd < 3 || fd >= 128)
    exit(-1);

  lock_acquire(&file_lock);

  struct file *f = thread_current()->fd[fd];
  if (!f)
  {
    lock_release(&file_lock);
    exit(-1);
  }

  unsigned pos = file_tell(f);

  lock_release(&file_lock);

  return pos;
}

void close(int fd)
{
  if (fd < 3 || fd >= 128)
    exit(-1);
  struct file *tmp = thread_current()->fd[fd];
  if (!tmp)
    exit(-1);

  thread_current()->fd[fd] = NULL;

  file_close(tmp);
  return;
}

// This two additional syscall functions (for project 1) causes kernel panic
// but i dont know why
// pintOS sucks
int fibonacci(int n)
{
  return 1;
}

int max_of_four_int(int a, int b, int c, int d)
{
  return 1;
}

unsigned int mmap(int fd, void *addr)
{
  return mm_map(fd, addr);
}

void munmap(unsigned int mapid)
{
  mm_free(mapid);
  return;
}
