#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "vm/page.h"

int process_execute(const char *file_name);
int process_wait(int);
void process_exit(void);
void process_activate(void);

bool expand_stack(void *addr, void *esp);
bool mm_fault_handler(struct pt_entry *pte);

#endif /* userprog/process.h */
