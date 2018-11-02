#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>

struct file_index{
	struct list_elem elem;
	int index;
	struct file* file;
};

void syscall_init (void);

#endif /* userprog/syscall.h */
