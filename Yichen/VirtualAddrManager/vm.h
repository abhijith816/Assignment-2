#define NUMFRAMES 256
#define NUMPAGES 256
#define	PAGESIZE 256

#include <stdio.h>
#include <stdlib.h>

typedef struct address{
	int logAddr;	//logical address found in text file
	int pagenum;	//page number (bits 8-15)
	int offset;	//offset (bits 0-7)
	int physAddr;
	int value;
} address;


void backingStore(address* a);
void pageTableLookup(address* a);
void extractor(address* a);
void initPageTable(void);
void getValue(address* a);
