#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "userprog/process.h"
#include "lib/kernel/list.h"
#include <string.h>

//mymy


static void syscall_handler (struct intr_frame *);
struct file* get_file(int file_index);
int add_file_index(char * file);
void delete_file_index(int file_index);
void sys_write(void *esp, void *eax);
void sys_tell(void * esp,void *eax);
void sys_seek(void * esp);
void sys_read(void * esp, void *eax);
void sys_close(void *esp);
void sys_open(void *esp,void * eax);
void sys_create(void * esp,void * eax);
void kill_thread(int status);
void verify_uaddr (void* ptr);


void verify_uaddr (void* ptr)
{
	if (ptr == NULL || !is_user_vaddr(ptr)) {
		kill_thread(-1);
	}
	if (!pagedir_get_page(thread_current()->pagedir, ptr))
	{
		kill_thread(-1);
	}
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
//  printf ("system call!\n");
//  thread_exit ();
  
	void * esp = f -> esp;
  	
	verify_uaddr(esp);
	int number = *((int *)esp);
	void * eax = &(f -> eax);
	
	if(number == SYS_HALT){
		shutdown_power_off();
		
	}else if(number == SYS_EXIT){
		sys_exit(esp);
	
	}else if(number == SYS_EXEC){
		sys_exec(esp,eax); 
	}else if(number == SYS_WAIT){
		sys_wait(esp,eax);
	}else if(number == SYS_CREATE){
		sys_create(esp,eax);
	}else if(number == SYS_REMOVE){
		verify_uaddr(esp+4);
		char * file_name = *((char **)(esp+4));
		verify_uaddr(file_name);
		lock_acquire(&file_access_lock); 
		*((bool *)eax) =  filesys_remove(file_name);
		lock_release(&file_access_lock); 
	}else if(number == SYS_OPEN){
		sys_open(esp,eax);
	}else if(number == SYS_FILESIZE){
		sys_filesize(esp,eax);
	}else if(number == SYS_WRITE){
		sys_write(esp,eax);
	}else if(number == SYS_SEEK){
		sys_seek(esp);
	}else if(number == SYS_TELL){
		sys_tell(esp,eax);
	}else if(number == SYS_CLOSE){
		sys_close(esp);
	}else if(number == SYS_READ){
		sys_read(esp,eax);
	}
}


void sys_exit(void *esp){

	verify_uaddr(esp+4);
	int status = *((int*)(esp+4));
	kill_thread(status);
}



void sys_exec(void *esp, void *eax){

	verify_uaddr(esp+4);
	char * file_name = *((char **)(esp+4));
	verify_uaddr(file_name);
	char * dummy;
	char * copy_name = (char *) malloc(strlen(file_name)+1);
	memcpy(copy_name,file_name,strlen(file_name)+1);
	copy_name = strtok_r(copy_name," \t",&dummy);
	lock_acquire(&file_access_lock); 
	
	if (filesys_open(copy_name)==NULL) {
		lock_release(&file_access_lock); 
		*((int *)eax) =-1;
	}
	
	else
	{
		lock_release(&file_access_lock); 
		tid_t child_tid = process_execute(file_name);
		
		//printf("[%d:TRYDOWN, %d]",thread_current()->tid,(thread_current()->load_wait).value);
		sema_down(&(thread_current()->load_wait));  
		
		//printf("[%d:DOWN]",thread_current()->tid);
		if (thread_current()->load_success == true)
			*((int *)eax) = child_tid;
		else *((int *)eax) = -1;
	}
	free(copy_name);
}


void sys_filesize(void *esp, void *eax){
	verify_uaddr(esp+4);
	int fd = *((char **)(esp+4));
	struct file * f = get_file(fd);
	lock_acquire(&file_access_lock); 
	*((int *)eax) =  file_length(f);
	lock_release(&file_access_lock); 
}


struct child * find_child(int tid){

	struct list_elem *e; 
	struct thread * t = thread_current();
	
	for (e = list_begin (&t->child_list); e != list_end (&t->child_list); e = list_next (e)){
		struct child * c = list_entry (e, struct child, elem);
		if(c->tid == tid){
			return c;
		}   
	}
	
	return NULL;
}


void sys_wait(void * esp,void * eax){

	verify_uaddr(esp+4);
	tid_t tid = *((tid_t *)(esp+4));
	struct child * c;
	
	if(tid==-1){
		*((int *)eax) = -1; 
		return;
	}
	c = find_child(tid);
	
	if(c==NULL){
		*((int *)eax) = -1; 
		return;
	}
	
	struct dead_thread * t = NULL;
	lock_acquire(&wait_lock);
	
	while((t = is_dead(tid))==NULL){
		cond_wait(&wait_cond,&wait_lock);
	}
	
	char * file_name;
	char * dummy;
	int exit_status = t->exit_status;
	file_name = strtok_r(t->name," \t",&dummy);
	printf("%s: exit(%d)\n",file_name, exit_status);
	
	list_remove(&(c->elem));
	free(t->name);
	
	//free(t); it doesn't harm any tc but multi-oom. it cause kernel panic to oom
	
	free(c);
	lock_release(&wait_lock);
	*((int *)eax) = exit_status; 
}


void sys_write(void *esp, void *eax){

	verify_uaddr(esp+4);
	verify_uaddr(esp+8);
	verify_uaddr(esp+12);
	int fd = *((int*)(esp+4));
	char * buffer  = *((char **)(esp + 8));
	unsigned size = *((unsigned *)(esp +12));
	verify_uaddr(buffer);
	verify_uaddr(buffer + size);
	
	if(fd ==0 ) kill_thread(-1);
	if(fd == 1){
		putbuf(buffer,size);
		*((int *)eax) =  size;
	}else{
		
		struct file * f = get_file(fd);
		
		if(f==NULL) kill_thread(-1);
		
		lock_acquire(&file_access_lock); 
		
		*((int *)eax) =  file_write(f,buffer,size);
		lock_release(&file_access_lock); 
	}
}


void sys_tell(void * esp,void *eax){

	verify_uaddr(esp+4);
	int fd = *((int*)(esp+4));
	struct file * f = get_file(fd);
	*((int *)eax) = file_tell(f);
}


void sys_seek(void * esp){

	verify_uaddr(esp+4);
	verify_uaddr(esp+8);
	
	int fd = *((int*)(esp+4));
	unsigned position = *((unsigned *)(esp +8));
	struct file * f = get_file(fd);
	
	lock_acquire(&file_access_lock); 
	file_seek(f,position);
	lock_release(&file_access_lock); 
}


void sys_read(void * esp, void *eax){

	verify_uaddr(esp+4);
	verify_uaddr(esp+8);
	verify_uaddr(esp+12);
	
	int fd = *((int*)(esp+4));
	char * buffer  = *((char **)(esp + 8));
	unsigned size = *((unsigned *)(esp +12));
	
	verify_uaddr(buffer);
	
	verify_uaddr(buffer + size);
	if(fd ==1) kill_thread(-1);
	struct file * f = get_file(fd);
	if(f==NULL) kill_thread(-1);

	lock_acquire(&file_access_lock); 
	*((int *)eax) = file_read(f,buffer,size);
	lock_release(&file_access_lock); 

}


void sys_close(void *esp){

	verify_uaddr(esp+4);
	int fd = *((int*)(esp+4));
	struct file * f = get_file(fd);
	if (f ==NULL) kill_thread(-1);
	if (fd == 0 || fd ==1 ) kill_thread(-1); 
	lock_acquire(&file_access_lock); 
	file_close(f);
	lock_release(&file_access_lock); 
	delete_file_index(fd);
}


void sys_open(void *esp,void * eax){
	verify_uaddr(esp+4);
	char * file_name = *((char **)(esp+4));
	verify_uaddr(file_name);
	if(file_name == NULL) kill_thread(-1);
	lock_acquire(&file_access_lock); 
	struct file * f = filesys_open(file_name);
	lock_release(&file_access_lock); 
	if (f==NULL) *((int *)eax) = -1; 
	else *((int *)eax) = add_file_index(f);
}


void sys_create(void * esp,void * eax){
	verify_uaddr(esp+4);
	verify_uaddr(esp+8);
	char * file_name = *((char **)(esp+4));
	verify_uaddr(file_name);
	unsigned initial_size = *((unsigned *)(esp+8));
	
	if(file_name == NULL) kill_thread(-1);
	lock_acquire(&file_access_lock); 
	bool b =  filesys_create(file_name,initial_size);
	lock_release(&file_access_lock); 
	*((int *)eax) = b;
}


void kill_thread(int status){
	
	lock_acquire(&wait_lock);
	
	if (thread_current()->is_parent_alive) {
		
		struct dead_thread * d = (struct dead_thread *) malloc(sizeof(struct dead_thread));
		
		struct thread * cur = thread_current();
		
		d->name = (char *)malloc(strlen(cur->name)+1);
		memcpy(d->name,cur->name,strlen(cur->name)+1);
		
		d->tid = cur->tid;
		
		d->exit_status = status;
		list_push_back(&dead_list,&d->elem);
	}
	lock_release(&wait_lock);
	thread_exit();
}

struct file* get_file(int file_index){
	struct list_elem *e;
	struct thread * t = thread_current();
	
	for (e = list_begin (&t->fd); e != list_end (&t->fd); e = list_next (e)){
		struct file_index *f = list_entry (e, struct file_index, elem);
		if(f->index == file_index){
			return f->file;
		}
	}
	        return NULL;
}


int add_file_index(char * file){

	struct thread * cur = thread_current();
	struct file_index * f_in = (struct file_index * )malloc(sizeof(struct file_index));
	f_in->index =(cur ->f_index++);
	f_in->file = file;
	list_push_back(&cur->fd,&f_in->elem);
	return f_in -> index;
}

void delete_file_index(int file_index){

	//수정해야함
	struct list_elem *e;
	struct thread * t = thread_current();
	
	for (e = list_begin (&t->fd); e != list_end (&t->fd); e = list_next (e)){
		struct file_index *f = list_entry (e, struct file_index, elem);
		if(f->index == file_index){
			list_remove(e);
			free(f);
			return;
		}
	}
}

