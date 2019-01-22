#include "heap.h"
#include "debug.h"
#include "stdint.h"
#include "atomic.h"

struct MemBlock {
    int32_t sizeOfBlock;
    struct MemBlock* next;
};

static struct MemBlock* head = 0;
size_t heapSize;
size_t heapLeft;

void heapInit(void* base, size_t bytes) {
    head = (MemBlock*)base;
    head->sizeOfBlock = -(int32_t)bytes;
    head->next = 0;
    heapSize = bytes;
    heapLeft = heapSize;
}


void* malloc(size_t bytes) {
    if(bytes < 0){
        return nullptr;
    }
    else if(bytes > heapSize){
        return nullptr;
    }
    else if(bytes > heapLeft){
        return nullptr;
    }
        
    bytes += sizeof(MemBlock);
    
    if(bytes % 4 != 0){
        bytes += (4 - bytes%4);
    }  

    MemBlock* curr = head;
    while(curr != 0){
        if(curr->sizeOfBlock < 0){
            int32_t currAbs = -(curr->sizeOfBlock);
            if(currAbs >= (ssize_t)((int32_t)bytes + sizeof(MemBlock))){
                break;
            }
        }
        curr = curr->next;
    }
    if(curr == 0){
        return nullptr;
    }

    if(-(curr->sizeOfBlock) > (ssize_t)bytes){
        MemBlock* newBlock = (MemBlock*)((uintptr_t)curr + bytes);
        MemBlock* tempNode = curr->next;
        newBlock->sizeOfBlock = -(-(curr->sizeOfBlock) - bytes);
        newBlock->next = tempNode;
        curr->next = newBlock;
        curr->sizeOfBlock = bytes;
    }
    heapLeft -= bytes;
    //Debug::printf("MALLOC %d BYTES - ADDRESS: %p - HEAP LEFT: %d\n", bytes, (MemBlock*)curr, heapLeft);
    return (MemBlock*)((uintptr_t)curr + sizeof(MemBlock));

}

void free(void* p) {
    MemBlock* curr = (MemBlock*)((uintptr_t)p - sizeof(MemBlock));
    heapLeft += curr->sizeOfBlock;
    //Debug::printf("FREE %p - HEAP LEFT: %d\n", p, heapLeft);
    curr->sizeOfBlock = -(curr->sizeOfBlock);
    MemBlock* rightNode = curr->next;
    if(rightNode != 0){
        if(rightNode->sizeOfBlock < 0){
            curr->sizeOfBlock = curr->sizeOfBlock + rightNode->sizeOfBlock;
            MemBlock* tempNode = rightNode->next;
            curr->next = tempNode;
            rightNode->next = nullptr;
        }
    }
}

void* realloc(void* ptr, size_t bytes){

    int currSize;
    void* newPtr;   
	MemBlock* currNode = head;

    if(ptr == nullptr){
	    return malloc(bytes);
    }
    
    currSize = currNode->sizeOfBlock;
    if((ssize_t)bytes <= currSize){
        return ptr;
    }

    newPtr = malloc(bytes);
    
    int* dst = (int*)newPtr;
    int* src = (int*)ptr;

    
    ssize_t i;
    for(i = 0; i < currSize/4; i++){
        dst[i] = src[i];
    }    

    free(ptr);
    return newPtr;
}

/*****************/
/* C++ operators */
/*****************/

void* operator new(size_t size) {
    return malloc(size);
}

void operator delete(void* p) {
    return free(p);
}

void operator delete(void* p, size_t size) {
    return free(p);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete[](void* p) {
    return free(p);
}