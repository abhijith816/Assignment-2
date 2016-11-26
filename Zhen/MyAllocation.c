#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<unistd.h>
#include"MyAllocation.h"
#define MAX_PAGES_SWAPPED 1000
#define PAGE_SIZE_NEW 4*1024

metadata_ptr metadata_list[MAX_THREADS];
static void* MEM_HEAD = NULL;
static int crntThread = 0;

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

char swapSpace[1+(16*1024*1024)] = {0};
struct _metadata *swap[MAX_PAGES_SWAPPED];
char *swapPageOffset;

//Initialization function for swapp file
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



void* init(){
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
    metadata_list[i]->size = MEM_SIZE - MAX_THREADS * PAGE_SIZE;
    metadata_list[i]->free = 1;
    metadata_list[i]->next = NULL;
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
        MEM_HEAD = init();
        initializeSwapSpace();
    }

    /*Allocating memory for current thread*/
	printf("Allocating size %d Bytes at %s, line %d\n", size, file, line);
    ptr = metadata_list[crntThread];
    while(ptr && !(ptr->free && ptr->size >= size)){
        ptr = ptr-> next;
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
