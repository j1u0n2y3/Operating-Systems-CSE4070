#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);

#define bool _Bool
struct lock file_lock;

void exit(int status);
int wait(int pid);
unsigned int read(int fd, void *buf, unsigned int nbytes);
unsigned int write(int fd, void *buf, unsigned int nbytes);
int fibonacci(int n);
int max_of_four_int(int x, int y, int z, int w);
/*****/
bool create(const char *fp, unsigned int size);
bool remove(const char *fp);
int open(const char *fp);
void close(int fd);
int filesize(int fd);
void seek(int fd, unsigned int pos);
unsigned int tell(int fd);

#endif /* userprog/syscall.h */
