//#define DATA_TYPE pthread
#include <ucontext.h> 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include<malloc.h>
#include<unistd.h>
#include "my_pthread_t.h"

#define RUNTIME 25000

int num_threads = 0;
int runningthreads = 0;
int waitingthreads = 0;
int waitingid = 0;
int currentrunningid = -1;
pthread * currentthread = NULL;
int isThreadRunning = 0;

struct _metadata{
	int threadId;
	void *start;
	void *end;
	int count;
	int nextPage;
		
    size_t size;
    int free;
    metadata_ptr next;
    char data[1];
};

metadata_ptr metadata_list[MAX_THREADS];
metadata_ptr library_data;
void* MEM_HEAD = NULL;
char swapSpace[1+(16*1024*1024)] = {0};
struct _metadata *swap[MAX_PAGES_SWAPPED];
char *swapPageOffset;


void init()
{
	my_pthread_yield();
}

void rq()
{
	nl[0] = my_thread_running_list;
	nl[1] = my_thread_running_list;
}

//This method schedules the next thread that is supposed to run. The interrupt must be raised every some particular time so that after a time slice a new thread gets a chance to run. 
void scheduler(int sig)
{
	printf("Inside the scheduler function \n");
	//if(runningthreads > 0)
	 my_pthread_yield();
	
	//Check if the running queue is not empty then select a new thread to run from that. For now we can do random order or FCFS but later implement multilevel feedback priority queue.
}

void prescheduler(void (*func)(void))
{
	printf("In Prescheduler function \n");
	struct sigaction interrupt_scheduler;
	struct itimerval it;
	
	interrupt_scheduler.sa_handler = scheduler;
	sigemptyset(&interrupt_scheduler.sa_mask);
	interrupt_scheduler.sa_flags = 0;
	
	sigaction(SIGALRM,&interrupt_scheduler,NULL);
	
	it.it_interval.tv_sec = 0;
 	it.it_interval.tv_usec = RUNTIME;
 	it.it_value.tv_sec = 0;
 	it.it_value.tv_usec = RUNTIME;
 	
 	setitimer(ITIMER_REAL, &it,NULL);
 	
 	func();
 	
 	//Since the function is done. Remove the thread from the running queue.
 	int c = 0;
 	for(c = currentrunningid; c< runningthreads -1; c++)
 		my_thread_running_list[c] = my_thread_running_list[c+1];
 		
 	//Reduce the number of running threads	
 	runningthreads--;
 	
 	printf("End of function in thread \n");
 	printf("The value of number of running threads in prescheduler is : %d \n",runningthreads);
 	if(runningthreads > 0)
	 	my_pthread_yield();
}

//The first argument is the buffer which stores the new thread information that is created successfully in this method. 
int my_pthread_create(pthread *thread,  void (*function)(void), void * arg)
{
	if(num_threads == MAX_THREADS)
		return 1;
	
	ucontext_t curr;
	getcontext(&curr);
	
	getcontext(&((*thread).mycontext));
	
	//Now have to makecontext to bind the context created above and the function passed as argument.
	(*thread).threadid = num_threads;
	(*thread).mycontext.uc_link=0;
	(*thread).mycontext.uc_stack.ss_sp=myallocate(MEM, __FILE__, __LINE__, LIBRARYREQ);
	(*thread).mycontext.uc_stack.ss_size = MEM;
	(*thread).mycontext.uc_stack.ss_flags = 0;
	(*thread).waitjoin = -1;	
	makecontext(&((*thread).mycontext), (void*)&prescheduler,1,(void*)function);
	
	//function();
	++num_threads;
	
	my_thread_list[num_threads] = thread;
	//Adding the newly created thread to the running queue.
	my_thread_running_list[runningthreads] = thread;
	runningthreads++;	
	
	//swapcontext(&curr,&((*thread).mycontext));
	
	printf("Thread created successfully\n");
	
	//prescheduler();
	 
	return 0;
}

//Scheduler will call this method after a certain time and we have to schedule another thread to run
//FCFS for now from the running threads
void my_pthread_yield()
{
	printf("Inside Yielding \n");

	if(isThreadRunning)
	{
		printf("Switching to main thread \n");
		swapcontext(&((*currentthread).mycontext),&main_context);		
	}
	else
	{
		currentrunningid = (currentrunningid + 1) % num_threads;		
		currentthread = my_thread_running_list[currentrunningid];
		printf("The threadid of the current running thread is : %d \n",(*currentthread).threadid);
		isThreadRunning = 1;
		swapcontext(&main_context,&((*currentthread).mycontext));
		isThreadRunning = 0;		
		printf("Return from swapcontext \n");
	}

	printf("Yielding is done \n");
}

void my_pthread_exit(void *value_ptr)
{
	//Exit the current thread.
	printf("thread %d is inside exiting\n", currentrunningid);
	if(currentthread->waitjoin != -1){
		//set the waitfor join thread runnable
		my_thread_list[currentthread->waitjoin]->status = value_ptr;
		//TODO set my_pthread_list[currentthread->waitjoin] to run
		my_thread_waiting_list[currentthread->waitjoin] = NULL;
		my_thread_running_list[runningthreads] =  my_thread_list[currentthread->waitjoin]; 
		runningthreads++;	
	}
	free(currentthread->mycontext.uc_stack.ss_sp);
	free(currentthread);	
	int c = 0;
 	for(c = currentrunningid; c< runningthreads -1; c++)
 		my_thread_running_list[c] = my_thread_running_list[c+1];	
	runningthreads--;
	my_thread_list[currentrunningid] = NULL;	
	num_threads--;
	isThreadRunning = 0;
	my_pthread_yield();
}
int my_pthread_join(pthread *thread,  void **value_ptr){
	printf("thread %d is inside join\n", currentrunningid);
	thread->waitjoin = currentthread->threadid;	
	value_ptr = &(currentthread->status);
	// TODO set currentthread waiting
	//
	my_thread_waiting_list[currentthread->waitjoin] = thread; 
	int c = 0;
	for(c = currentrunningid; c< runningthreads -1; c++)
 		my_thread_running_list[c] = my_thread_running_list[c+1];	
	runningthreads--;
	isThreadRunning = 0;
	my_pthread_yield();
	return 0;
}

void initialize()
{
	printf("Inside the initialize function \n");
	printf("The number of running threads is : %d \n",runningthreads);
	
	while(runningthreads > 0)	
		my_pthread_yield();
	
	printf("Initialization is done \n");
}

// Memory Library

//Initialization function for swap file
void initializeSwapSpace()
{
	char *startAddress;
	int allocationSize;

	startAddress = swapSpace;
	allocationSize = sizeof(metadata);

	int i;
	for(i=0; i<MAX_PAGES_SWAPPED; i++)
	{
		swap[i] = startAddress;
		startAddress += allocationSize;
	}

	swapPageOffset = startAddress;

	printf("swapSpace : %p, swap[0]: %d , swap[1]: %d \n", swapSpace, swap[0] , swap[1]);

	allocationSize = 4 * 1024;

	for(i=0; i<MAX_PAGES_SWAPPED; i++)
	{
		swap[i]->threadId = -1;
		swap[i]->start = swap[i]->end = startAddress;
		startAddress += allocationSize;

		swap[i]->count = 0;

		int j= 0;

		for(j=0; j<100; j++)
		{
			swap[i]->free = 1;
		}

		swap[i]->nextPage = -1;
	}

	printf("threadID : %d, count: %d  \n", swap[1]->threadId, swap[1]->count);
}

int findPageInMemoryToSwap(int currentRunningThreadId)
{
	int count = 0;
	srand(time(NULL));
	int pageNo = 1;
	pageNo = rand() % MAX_THREADS;

	while(1)
	{
		if(metadata_list[pageNo]->threadId != currentRunningThreadId)
		{
			return pageNo;
		}

		pageNo++;

		if(pageNo == MAX_THREADS)
		{
			pageNo = 0;
		}

		count++;
		if(count == 100)
		{
			break;
		}
	}

	return -1;

}

int findSwapPage()
{
	int i =0;

	for(i=0; i<MAX_PAGES_SWAPPED; i++)
	{
		if(swap[i]->threadId == -1)
		{
			return i;
		}
	}

	return -1;
}

void resetMainPage(int pageNumber)
{
	metadata_list[pageNumber]->threadId = -1;
	metadata_list[pageNumber]->end = metadata_list[pageNumber]->start;
	metadata_list[pageNumber]->count = 0;

	int j=0;
	for(j=0; j<100; j++)
	{
		metadata_list[pageNumber]->free = 1;
	}

	metadata_list[pageNumber]->nextPage = -1;

}

int initiateSwapMainToSwap(int mainPage)
{
	int swapPage = findSwapPage();

	if(swapPage == -1)
	{
		printf("Swap memory full\n");
		return -1;
	}

	char * swapPageLocation = swapPageOffset + swapPage * PAGE_SIZE_NEW;

	memcpy(swapPageLocation, metadata_list[mainPage]->start, PAGE_SIZE_NEW);

	memcpy(swap[swapPage], metadata_list[mainPage], sizeof(metadata));

	resetMainPage(mainPage);

	return 0;
}



void* init_memory(){
	void* p = NULL;
    int i;

    printf("Initizlizing Memory of %lf Bytes...\n", MEM_SIZE);
    /*Allocate aligned memory*/
	p = (void*)memalign(sysconf(_SC_PAGE_SIZE), MEM_SIZE);	
    /*Allocate one page for each thread*/
    for(i = 0; i < MAX_THREADS; ++i){
        metadata_list[i] = p + i * PAGE_SIZE ;
        metadata_list[i]->size = PAGE_SIZE - sizeof(metadata);
        metadata_list[i]->free = 1;
        metadata_list[i]->next = NULL;
    } 
    /*The rest memory*/
    metadata_list[i] = p + i * PAGE_SIZE;
    metadata_list[i]->size = MEM_SIZE - MAX_THREADS * PAGE_SIZE - LIBRARY_SIZE;
    metadata_list[i]->free = 1;
    metadata_list[i]->next = NULL;

	library_data = metadata_list[i]->data + metadata_list[i]->size;
	library_data->size = LIBRARY_SIZE - sizeof(metadata);
	library_data->free = 1;
	library_data->next = NULL;	
	return p;	
}

void* split(metadata_ptr old_ptr, size_t size){
    metadata_ptr new_ptr;
    new_ptr = (metadata_ptr)(old_ptr->data + size);
    new_ptr->size = old_ptr->size - size - sizeof(metadata);
    new_ptr->free = 1;
    new_ptr->next = old_ptr->next; 
    old_ptr->size = size;
    old_ptr->next = new_ptr; 
    return old_ptr;
}

void* myallocate(size_t size, char* file, int line, int threadReq){
    metadata_ptr ptr;
    if(MEM_HEAD == NULL){// Memory has not been initialized
        MEM_HEAD = init_memory();
        initializeSwapSpace();
    }
	if(threadReq == THREADREQ){
		/*Allocating memory for current thread*/
		printf("Allocating size %d Bytes at %s, line %d\n", size, file, line);
		ptr = metadata_list[currentrunningid];
		while(ptr && !(ptr->free && ptr->size >= size)){
			ptr = ptr-> next;
		}
	} 
	else if(threadReq == LIBRARYREQ){
		printf("Library allocation for thread creation\n");
		ptr = library_data;
		while(ptr && !(ptr->free && ptr->size >= size)){
			ptr = ptr->next;
		}
	}
    if(ptr != NULL){
        split(ptr, size);
        ptr->free = 0;
    }
    return ptr->data;
}

void mydeallocate(void* data_ptr, char* file, int line, int threadReq){
    metadata_ptr ptr = data_ptr - sizeof(metadata);
    ptr->free = 1; 
	printf("Deallocating %s, line %d\n", file, line);
}


