#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
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
		f->eax = process_wait(*(int *)((unsigned char *)esp + 4 * 1));
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

	default:
		// printf("not a system call for project 1.\n");
		break;
	}
}

void exit(int status)
{
	// 'exit' function needs to be explicit unlike exec and wait,
	// (that is, we cannot just use thread_exit() function in "../process.h",)
	// because many other codes refer to / call this function..
	// COMPILE ERROR ERROR ERROR ERROR ERROR ERROR . . . . . . . (-_ -);;
	thread_current()->exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status);
	thread_exit();
}

unsigned int read(int fd, void *buf, unsigned int nbytes)
{
	unsigned int bytes = 0;
	if (fd == STDIN_FILENO)
	{
		for (unsigned int i = 0; i < nbytes; i++)
		{
			char c = input_getc();
			if (c == '\0')
				break;

			bytes++;
			*(char *)buf = c;
			buf = (char *)buf + 1;
		}
		*(char *)buf = '\0';
	}
	else
	{
		// printf("can only read from stdin in project 1.\n");
		bytes = -1; // 0?
	}
	return bytes;
}

unsigned int write(int fd, void *buf, unsigned int nbytes)
{
	unsigned int bytes = 0;
	if (fd == STDOUT_FILENO)
	{
		bytes = nbytes;
		putbuf(buf, nbytes);
	}
	else
	{
		// printf("can only write on stdout in project 1.\n");
		bytes = -1; // 0?
	}
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
