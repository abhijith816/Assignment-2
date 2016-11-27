#include "vm.h"
#include "MyAllocation.h"

int pageTable[NUMPAGES];
int physMem[NUMFRAMES][PAGESIZE];
int pageFault=0;
int frameNum=0;
void* pageAddress[NUMPAGES];   //This record address of simulated 
pageMetaData pageMetaDataArr[NUMPAGES];

//Building mapping from physical address to virtual address(Pagelize)
void formatSimulatedMemory(void){
	for (int i = 0; i < NUMPAGES; ++i)
	{
		pageAddress[i]=malloc(PAGESIZE);
		pageMetaDataArr[i].isFree=0;
		pageMetaDataArr[i].threadId=-1;
		pageMetaDataArr[i].pageAddress=pageAddress[i];
	}
}

int isMemoryFull(void){
	for(int i=0;i<NUMPAGES;i++){
		if (pageMetaDataArr[i].isFree==1)
		{
			return 0;
		}
	}
	return 1;
}

void* requestForFreePage(int threadId){
	if(isMemoryFull()==1){						//if memory is full
		return NULL;                                
	}

	for(int i=0;i<PAGESIZE;i++){
		if(pageMetaDataArr[i].isFree==1){
			pageMetaDataArr[i].isFree=0;
			pageMetaDataArr[i].threadId=threadId;
			return pageMetaDataArr[i].pageAddress;
		}
	}
	
}

void initPageTable(void){
	int i;
	for(i=0; i<NUMPAGES; i++)
		pageTable[i]=-1;
	return;
}

void extractor(address* a)
{
	a->pagenum = (a->logAddr & 0x000FF000) >>(12);
	a->offset = a->logAddr & 0x00000FFF;
	return;
}

/* This function looks up the framenum from a pagenumbers, if the framenumber is not in the pagetable
 * we increase the pagefault flag and call the thread to bring the page from memory.
 */
void pageTableLookup(address* a){
	if( pageTable[a->pagenum] == -1){	
		pageTable[a->pagenum]=frameNum;
		backingStore(a);
		frameNum++;
		pageFault++;
	}

	getValue(a);

	a->physAddr=pageTable[a->pagenum]*PAGESIZE+a->offset;
	
	return;
}

void backingStore(address *a){

}

// Reads the value from the physical memory that corresponds to the logical address
void getValue(address* a){
	a->value=physMem[pageTable[a->pagenum]][ a->offset ];//Read the value from physical memory
}
