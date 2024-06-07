#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"

#define bool _Bool

struct lock file_lock;

void syscall_init(void);
void exit(int status);
int exec(const char *cmd_line);
int wait(int pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned int tell(int fd);
void close(int fd);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);
/*****/
unsigned int mmap(int fd, void *addr);
void munmap(unsigned int mapid);

#endif /* userprog/syscall.h */
