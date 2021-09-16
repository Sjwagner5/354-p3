///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        heapAlloc.c
// This File:        heapAlloc.c
// Other Files:      heapAlloc.c
// Semester:         CS 354 Fall 2019
//
// Author:           Sam Wagner
// Email:            sjwagner5@wisc.edu
// CS Login:         samw
//
/////////////////////////// OTHER SOURCES OF HELP /////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:   avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of 
//                   of any information you find.
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "heapAlloc.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {
        int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */

blockHeader *heapStart = NULL;
blockHeader *lastBlock; // this will point to the block that was allocated most recently

void splitBlock(blockHeader *current, int newBlockSize){

	blockHeader *newFreeBlockHead;
	blockHeader *newFreeBlockFoot;
	newFreeBlockHead = (blockHeader*) ((char*)current + newBlockSize);// point to the start of the new free block
	
	if (newFreeBlockHead -> size_status != 1){
		newFreeBlockHead -> size_status = ((current -> size_status) - (current -> size_status % 8)) - newBlockSize;
	
		newFreeBlockHead -> size_status += 2;
		//get the size of the new free block and set the p bit to 1
		
		newFreeBlockFoot = (blockHeader*) ((char*)newFreeBlockHead + ((newFreeBlockHead -> size_status) - (newFreeBlockHead -> size_status % 8)));
		
		//point to end of free block
		newFreeBlockFoot = (blockHeader*) ((char*)newFreeBlockFoot - sizeof(blockHeader));//point to the start of the footer
		
		newFreeBlockFoot -> size_status = (newFreeBlockHead -> size_status) - (newFreeBlockHead -> size_status % 8);
	}
}

/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* allocHeap(int size) {	
	
	if (size <= 0){
		return NULL;
	}

	void *newBlock = NULL;// the start of the newly allocated block, which is the address we will return
	blockHeader *currentBlock;// this will keep track of the block we are currently looking at to try and allocate it

	int fullSize = size + sizeof(blockHeader); // size of the block plus it's header
	if (fullSize % 8 != 0){
		fullSize = fullSize + 8 - (fullSize % 8); // this will round the size to a multiple of 8
	}
	
	blockHeader* nextBlock;// will point to the next block after the allocated one

	if (lastBlock == NULL) {// if this is the first block allocated, allocate the block at the start of the heap and split the rest

		if (fullSize > 	((heapStart -> size_status) - (heapStart -> size_status % 8))) {// return null if the first block is too big
			return NULL;
		}

		currentBlock = (blockHeader*) ((void*)heapStart);// current block is at the beginning of the heap

		// start to split the rest of the block into a new free block		
		blockHeader *newFreeBlockHead;
		blockHeader *newFreeBlockFoot;

		newFreeBlockHead = (blockHeader*)((char*)currentBlock + fullSize);

		if (newFreeBlockHead -> size_status != 1) {// if the end of the heap is not reached, split the block
			
			newFreeBlockHead -> size_status = ((heapStart -> size_status) - (heapStart -> size_status % 8)) - fullSize;// change the new free block's size
																										  			   // to the heap size minus the new block
			newFreeBlockHead -> size_status += 2; // change the p bit to 1

			newFreeBlockFoot = (blockHeader*)((char*)newFreeBlockHead + ((newFreeBlockHead -> size_status) - (newFreeBlockHead -> size_status % 8)));
			// get to the end of the heap

			newFreeBlockFoot = (blockHeader*)((char*)newFreeBlockFoot - sizeof(blockHeader));// point to the start of the footer
			newFreeBlockFoot -> size_status = (newFreeBlockHead -> size_status) - (newFreeBlockHead -> size_status % 8);
		}
		currentBlock -> size_status = (fullSize + 3); // change the size to the size of the new block and the p and a bits both to 1
		
		lastBlock = (blockHeader*)((void*)currentBlock); // the last allocated block is the one we just allocated
		
		newBlock = (blockHeader*)((char*)currentBlock + sizeof(blockHeader));// new block pointer points to the start of the payload

		return newBlock;
	}
	
	currentBlock = (blockHeader*)((char*)lastBlock);// set current to the block that was most recently allocated

	do {// loop through until the last allocated block is reached
		if (currentBlock -> size_status == 1) {
			currentBlock = (blockHeader*) ((void*)heapStart);// if the end of the heap is reached, go to the front of the heap

		} else if (((currentBlock -> size_status) >= fullSize) && ((currentBlock -> size_status & 1) == 0)) {
					
			if (((currentBlock -> size_status)-(currentBlock-> size_status % 8)) > fullSize) {
				splitBlock(currentBlock, fullSize);// split the rest of the free block if possible
			}

			if (((currentBlock -> size_status) & 2) != 0) {
				currentBlock -> size_status = fullSize + 3;// if the p block was 1, set the new header to the size and p and a bits as 1
			} else {
				currentBlock -> size_status = fullSize + 1;// if the p block was 0, set the new header to the size and a bit as 1
			}
			
			nextBlock = (blockHeader*)((char*)currentBlock + fullSize);
			
			if (nextBlock -> size_status != 1 && ((nextBlock -> size_status) & 2) == 0) {
				nextBlock -> size_status += 2;// set p bit to 1	if not done already
			}
			
			lastBlock = (blockHeader*)((char*)currentBlock);// set the last block as the one just allocated
			newBlock = (blockHeader*)((char*)currentBlock + sizeof(blockHeader));// get the start of the payload
			return newBlock;			

		} else {
			currentBlock = (blockHeader*)((char*)currentBlock + ((currentBlock -> size_status) - ((currentBlock -> size_status) % 8)));
			// if this block is not big enough or allocated, go to the next one
		}
	} while (currentBlock != lastBlock);
	
    return NULL; // if the loop breaks, that means a large enough block was never found
}

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int freeHeap(void *ptr) {
	int freeingLastBlock = 0; //boolean that will store if the block to be freed is the last block pointer
	blockHeader* headPtr;
	headPtr = (blockHeader*) ptr;// cast ptr to a blockHeader rather than a void pointer

	if (headPtr == lastBlock) {
		freeingLastBlock = 1;
	}

	headPtr = (blockHeader*) ((char*)headPtr - sizeof(blockHeader)); // point ptr to the start of the block header rather than the payload

	blockHeader* endMark;// keep track of the end mark of the heap to see if ptr is within the heap size
	endMark = heapStart;
	int ptr_blockStart_match = 0;// boolean to ensure that ptr points to the start of a block

	while(endMark -> size_status != 1) {// loop until the endMark points to the end of the heap space
		if (endMark == headPtr) {
			ptr_blockStart_match = 1;
		}
		endMark = (blockHeader*) ((char*)endMark + ((endMark -> size_status) - ((endMark -> size_status) % 8)));
	}
	       
    if ((headPtr == NULL) || (headPtr < heapStart) || (headPtr > endMark) || (((headPtr -> size_status) & 1) == 0) || (ptr_blockStart_match == 0)) {
    	return -1;// return -1 if ptr is invalid in any way
	}

	blockHeader* nextBlockHead;// will point to the next block's header to update p bit
	blockHeader* newFooter;// will be the footer of the new free block


	if (((headPtr -> size_status) & 2) == 0) {// coalesce with previous block if necessary
		blockHeader* prevBlockFoot = (blockHeader*) ((char*)headPtr - sizeof(blockHeader));

		int blockToFreeSize = (headPtr -> size_status) - (headPtr -> size_status % 8); // get just the size of the block
		int prevBlockSize = prevBlockFoot -> size_status;
		headPtr = (blockHeader*) ((char*) headPtr - prevBlockSize);// point ptr to the previous block
		headPtr -> size_status = prevBlockSize + blockToFreeSize + 2;// set new free block's header to the size with p bit as 1

	} else { // if the previous block is not allocated, just change this block's a bit to 0
		headPtr -> size_status = (headPtr -> size_status) - 1;
		
	}

	nextBlockHead = (blockHeader*) ((char*) headPtr + ((headPtr -> size_status) - (headPtr -> size_status % 8)));
	//point to the next block
	
	if ((nextBlockHead -> size_status & 1) == 0) {// coalesce with next block if necessary
		int newBlockSize = (headPtr -> size_status - (headPtr -> size_status % 8)) + (nextBlockHead -> size_status - (nextBlockHead -> size_status % 8));
		//get the total new block's size

		headPtr -> size_status = newBlockSize + 2;// set new header to size with p bit as 1
		if (nextBlockHead == lastBlock) {
			freeingLastBlock = 1;// if the next block that is being coalesced was the last block pointer, change the last block pointer
		}
	}

	newFooter = (blockHeader*) ((char*) headPtr + ((headPtr -> size_status) - (headPtr -> size_status % 8)));
	newFooter = (blockHeader*) ((char*) newFooter - sizeof(blockHeader));// point to the new footer of this free block
	newFooter -> size_status = (headPtr -> size_status) - (headPtr -> size_status % 8);

	nextBlockHead = (blockHeader*)((char*)headPtr + (headPtr -> size_status)-(headPtr -> size_status % 8));//point the next block header to the next allocated block
	
	if (nextBlockHead -> size_status != 1 && ((nextBlockHead -> size_status) & 2) == 2) {
		nextBlockHead -> size_status -= 2;// change next block's p bit to 0
	}

	if (freeingLastBlock) {
		lastBlock = (blockHeader*)((char*)headPtr);
	}
	return 0;
}

/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int initHeap(int sizeOfRegion) {         

    static int allocated_once = 0; //prevent multiple initHeap calls

    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    int allocsize; // size of requested allocation including padding
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((char*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
}         
                 
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dumpMem() {  

    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;
    
    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
		
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
}  
