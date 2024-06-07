#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/string.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init(&file_lock);
}

static void syscall_handler(struct intr_frame *f UNUSED)
{
	unsigned int *esp = (unsigned int *)(f->esp);
	unsigned int syscall_num = *(esp + 0);

	switch (syscall_num)
	{
	case SYS_HALT:
		shutdown_power_off();
		break;

	case SYS_EXIT:
		if ((int *)((unsigned char *)esp + 4 * 1) == NULL ||
			is_user_vaddr((int *)((unsigned char *)esp + 4 * 1)) == false)
			exit(-1);
		exit(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_EXEC:
		if ((char **)((unsigned char *)esp + 4 * 1) == NULL ||
			is_user_vaddr((char **)((unsigned char *)esp + 4 * 1)) == false)
			exit(-1);
		f->eax = process_execute(*(char **)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_WAIT:
		if ((int *)((unsigned char *)esp + 4 * 1) == NULL ||
			is_user_vaddr((int *)((unsigned char *)esp + 4 * 1)) == false)
			exit(-1);
		f->eax = wait(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_READ:
		for (int i = 1; i <= 3; i++)
		{
			if ((void **)((unsigned char *)esp + 4 * i) == NULL ||
				is_user_vaddr((void **)((unsigned char *)esp + 4 * i)) == false)
				exit(-1);
		}
		f->eax = read(*(int *)((unsigned char *)esp + 4 * 1),
					  *(void **)((unsigned char *)esp + 4 * 2),
					  *(unsigned *)((unsigned char *)esp + 4 * 3));
		break;

	case SYS_WRITE:
		for (int i = 1; i <= 3; i++)
		{
			if ((void **)((unsigned char *)esp + 4 * i) == NULL ||
				is_user_vaddr((void **)((unsigned char *)esp + 4 * i)) == false)
				exit(-1);
		}
		f->eax = write(*(int *)((unsigned char *)esp + 4 * 1),
					   *(void **)((unsigned char *)esp + 4 * 2),
					   *(unsigned *)((unsigned char *)esp + 4 * 3));
		break;

	case SYS_FIBONACCI:
		if ((int *)((unsigned char *)esp + 4 * 1) == NULL ||
			is_user_vaddr((int *)((unsigned char *)esp + 4 * 1)) == false)
			exit(-1);
		f->eax = fibonacci(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_MAX_OF_FOUR_INT:
		for (int i = 1; i <= 4; i++)
		{
			if ((int *)((unsigned char *)esp + 4 * i) == NULL ||
				is_user_vaddr((int *)((unsigned char *)esp + 4 * i)) == false)
				exit(-1);
		}
		f->eax = max_of_four_int(*(int *)((unsigned char *)esp + 4 * 1),
								 *(int *)((unsigned char *)esp + 4 * 2),
								 *(int *)((unsigned char *)esp + 4 * 3),
								 *(int *)((unsigned char *)esp + 4 * 4));
		break;
	/*****/
	case SYS_CREATE:
		if ((unsigned char *)esp + 4 * 2 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 2) == false)
			exit(-1);
		f->eax = create(*(char **)((unsigned char *)esp + 4 * 1), *(unsigned *)((unsigned char *)esp + 4 * 2));
		break;

	case SYS_REMOVE:
		if ((unsigned char *)esp + 4 * 1 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 1) == false)
			exit(-1);
		f->eax = remove(*(char **)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_OPEN:
		if ((unsigned char *)esp + 4 * 1 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 1) == false)
			exit(-1);
		f->eax = open(*(char **)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_CLOSE:
		if ((unsigned char *)esp + 4 * 1 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 1) == false)
			exit(-1);
		close(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_FILESIZE:
		if ((unsigned char *)esp + 4 * 1 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 1) == false)
			exit(-1);
		f->eax = filesize(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	case SYS_SEEK:
		for (int i = 1; i <= 2; i++)
		{
			if ((unsigned char *)esp + 4 * i == NULL || is_user_vaddr((unsigned char *)esp + 4 * i) == false)
				exit(-1);
		}
		seek(*(int *)((unsigned char *)esp + 4 * 1), *(unsigned *)((unsigned char *)esp + 4 * 2));
		break;

	case SYS_TELL:
		if ((unsigned char *)esp + 4 * 1 == NULL || is_user_vaddr((unsigned char *)esp + 4 * 1) == false)
			exit(-1);
		f->eax = tell(*(int *)((unsigned char *)esp + 4 * 1));
		break;

	default:
		// printf("not a system call for this project.\n");
		exit(-1);
	}
}

int wait(int pid)
{
	return process_wait(pid);
}

void exit(int status)
{
	thread_current()->exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status);

	for (int i = 0; i < 128; i++)
	{
		if (thread_current()->fd[i])
			close(i);
	}

	struct list *cur_list = &(thread_current()->child_list);
	for (struct list_elem *iter = list_begin(cur_list);
		 iter != list_end(cur_list);
		 iter = list_next(iter))
		wait((list_entry(iter, struct thread, child_elem))->tid);

	thread_exit();
	return;
}

unsigned int read(int fd, void *buf, unsigned int nbytes)
{
	if (buf == NULL || is_user_vaddr(buf) == false)
		exit(-1);

	lock_acquire(&file_lock);
	unsigned int bytes = 0;
	if (fd == 0)
	{
		char c;
		for (int i = 0; (i < nbytes) && ((c = input_getc()) != '\0'); i++)
		{
			++bytes;
			*((char *)buf) = c;
			buf = (char *)buf + 1;
		}
		*((char *)buf) = '\0';
	}
	else if (3 <= fd && fd < 128)
	{
		if (thread_current()->fd[fd] == NULL)
		{
			lock_release(&file_lock);
			exit(-1);
		}

		bytes = file_read(thread_current()->fd[fd], buf, nbytes);
	}
	else
	{
		lock_release(&file_lock);
		exit(-1);
	}
	lock_release(&file_lock);

	return bytes;
}

unsigned int write(int fd, void *buf, unsigned int nbytes)
{
	if (buf == NULL || is_user_vaddr(buf) == false)
		exit(-1);

	lock_acquire(&file_lock);
	unsigned int bytes = 0;
	if (fd == 1)
	{
		bytes = nbytes;
		putbuf(buf, nbytes);
	}
	else if (3 <= fd && fd < 128)
	{
		if (thread_current()->fd[fd] == NULL)
		{
			lock_release(&file_lock);
			exit(-1);
		}
		if ((thread_current()->fd[fd])->deny_write)
			file_deny_write(thread_current()->fd[fd]);

		bytes = file_write(thread_current()->fd[fd], buf, nbytes);
	}
	else
	{
		lock_release(&file_lock);
		exit(-1);
	}
	lock_release(&file_lock);

	return bytes;
}

int fibonacci(int n)
{
	if (n < 0)
	{
		// printf("first number cannot be negative.\n");
		exit(-1);
	}
	if (n == 0)
		return 0;
	if (n == 1)
		return 1;

	int fib[3];
	fib[0] = 0;
	fib[1] = 1;
	for (int i = 2; i <= n; i++)
	{
		fib[2] = fib[0] + fib[1];
		fib[0] = fib[1];
		fib[1] = fib[2];
	}
	return fib[2];
}

inline int max_int(int x, int y)
{
	return (x > y ? x : y);
}

int max_of_four_int(int x, int y, int z, int w)
{
	return max_int(max_int(x, y), max_int(z, w));
}
/*****/
bool create(const char *fp, unsigned int size)
{
	if (fp == NULL || is_user_vaddr(fp) == false)
		exit(-1);
	return filesys_create(fp, size);
}

bool remove(const char *fp)
{
	if (fp == NULL || is_user_vaddr(fp) == false)
		exit(-1);
	return filesys_remove(fp);
}

int open(const char *fp)
{
	if (fp == NULL || is_user_vaddr(fp) == false)
		exit(-1);

	lock_acquire(&file_lock);
	struct file *f = filesys_open(fp);
	if (!f)
	{
		lock_release(&file_lock);
		return -1;
	}
	if (!strcmp(thread_name(), fp))
		file_deny_write(f);

	int idx = -1;
	for (int i = 3; i < 128; i++)
	{
		if (!(thread_current()->fd[i]))
		{
			idx = i;
			thread_current()->fd[i] = f;
			break;
		}
	}
	lock_release(&file_lock);
	return idx;
}

void close(int fd)
{
	if (fd < 3 || fd >= 128)
		exit(-1);
	struct file *tmp = thread_current()->fd[fd];
	if (tmp == NULL)
		exit(-1);
	thread_current()->fd[fd] = NULL;
	file_close(tmp);
	return;
	/* At first I didn't set the temporal variable 'tmp' and
		set 'thread_current()->fd[fd]' NULL itself then call file_close. . . . . .
			(rox-child, rox-multichild, multi-oom) */
}

int filesize(int fd)
{
	if (fd < 3 || fd >= 128)
		exit(-1);
	if (thread_current()->fd[fd] == NULL)
		exit(-1);
	return file_length(thread_current()->fd[fd]);
}

void seek(int fd, unsigned int pos)
{
	if (fd < 3 || fd >= 128)
		exit(-1);
	if (thread_current()->fd[fd] == NULL)
		exit(-1);
	file_seek(thread_current()->fd[fd], pos);
	return;
}

unsigned int tell(int fd)
{
	if (fd < 3 || fd >= 128)
		exit(-1);
	if (thread_current()->fd[fd] == NULL)
		exit(-1);
	return file_tell(thread_current()->fd[fd]);
}