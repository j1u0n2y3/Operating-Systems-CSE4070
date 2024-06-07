#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);

void exit(int status);
unsigned int read(int fd, void *buf, unsigned int nbytes);
unsigned int write(int fd, void *buf, unsigned int nbytes);
int fibonacci(int n);
int max_of_four_int(int x, int y, int z, int w);

#endif /* userprog/syscall.h */
