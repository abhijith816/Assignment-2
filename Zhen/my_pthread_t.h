/// In this file maintain all the information that is only absolutely necessary else the preprocessor load will increase.

//Need a list of running and waiting queue. When a thread is created it has to be present in the running queue.

#include <ucontext.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define NUMFRAMES 256
#define NUMPAGES 256
#define	PAGESIZE 4096
#define MEM 64000
#define MAX_THREADS 10
#define MAX_PAGES_SWAPPED 1000
#define PAGE_SIZE_NEW 4*1024
#define LIBRARYREQ 0
#define THREADREQ 1
#define LIBRARY_SIZE 640000
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define MEM_SIZE 8e6
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
typedef struct _metadata metadata;
typedef metadata* metadata_ptr;
typedef struct {
	int threadid;
	ucontext_t mycontext;
	int waitjoin;// Waitting thread id;
	void * status;	
	void * stack;
	int priority;
	float timeslice;
	clock_t starttime;
	
}pthread;

typedef struct address{
	int logAddr;	//logical address found in text file
	int pagenum;	//page number (bits 8-15)
	int offset;	//offset (bits 0-7)
	int physAddr;
	int value;
} address;


typedef struct pagemeta{
	int isFree;
	int threadId;
	void* pageAddress;   //address of beginning of the page.
} pagemeta;

extern int finished;
extern int numthreads;
extern int runningid;
extern int waitingid;

static ucontext_t main_context;

//Maintaining an array of pthreads so that we can keep track of them. 
static pthread * my_thread_list[MAX_THREADS];

//Running queue
static pthread * my_thread_running_list[MAX_THREADS];

static pthread * nl[2];

//Waiting queue
static pthread * my_thread_waiting_list[MAX_THREADS];

extern void init();

/* Declaring all the functions that we are going to use in here*/

//Function to create a thread for a given function given as argument. This function returns 0 on success and on error returns the error id.
extern int my_pthread_create(pthread *thread,  void (*function)(void), void * arg);

//Call to scheduler to swap the current thread
extern void my_pthread_yield();

//End the thred that calls this function . If the argument is not null then the return value from the thread will be saved.
extern void pthread_exit(void *value_ptr);


//Call to the thread lib to make sure that the calling thread will not execute until the one it references exits. If the value_ptr is not null then return value of the exiting thread will be passed back.
extern int my_pthread_join(pthread *thread, void **value_ptr);

void* init_memory();
extern void* myallocate(size_t x, char* file, int line, int threadReq);
extern void mydeallocate(void* x, char* file, int line, int threadReq);

void backingStore(address* a);
void pageTableLookup(address* a);
void extractor(address* a);
void initPageTable(void);
void getValue(address* a);

void formatSimulatedMemory(void);
int isMemoryFull(void);
void* requestForFreePage(int threadId);
