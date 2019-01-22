#ifndef _REFS_H_
#define _REFS_H_

#include "machine.h"
#include "debug.h"
#include "atomic.h"


template <typename T>
struct RefBlock {
    int count;
    T* ptr;
};

template <typename T>
class StrongPtr {
public:
    RefBlock<T>* block = 0;

    StrongPtr() {  //Default constructor
	this->block = nullptr;   //Point to null
    }

    explicit StrongPtr(T* ptr) { //Pointer to dynamic alloc thread
	if(ptr == nullptr){
	    this->block = nullptr;
        }
	else{
 	    block = new RefBlock<T>();
	    block->count = 1;
	    block->ptr = ptr;
        }
    }

    StrongPtr(const StrongPtr& src) { //copy constructor
	if(src.block == nullptr){
	    if(this->block != nullptr){
		this->block->count--;
		if(this->block->count <= 0){
		    this->~StrongPtr();
                }
            }
	    this->block = nullptr;
        }
	else{
	    if(this->block == nullptr){
		this->block = src.block;
	        this->block->count++;
            }
	    else{
	        this->~StrongPtr();
            }
        }
    } 

    ~StrongPtr() { //Destructor
	if(this->block == nullptr){
	    return;
        }
	this->block->count--;
	if(this->block->count <= 0){
	    delete this->block->ptr;
	    delete this->block;
        }
    }

    T* operator -> ()  { //return access to referenced object
	return this->block->ptr;
    }

    bool isNull() const { //check if contains null
	if(this->block == nullptr){
  	    return true;
        }
	else{
	    return false;
        }
    }

    void reset() { //point current strongptr to null
	if(this->block == nullptr){
	    return;
        }
	this->block->count--;
	if(this->block->count <= 0){
	    this->~StrongPtr();
        }
        this->block = nullptr;
    }

    StrongPtr<T>& operator = (const StrongPtr& src) {
 	if(src.block == nullptr){
	    if(this->block != nullptr){
	        this->block->count--;
	        if(this->block->count <= 0){
		    this->~StrongPtr();
                }
            }
	    this->block = nullptr;
        }
	else{
	    if(this->block == nullptr){
		this->block = src.block;
	        this->block->count++;
            }
	    else{
	        this->block->count--;
	        if(this->block->count <= 0){
		    this->~StrongPtr();
                }
		this->block = src.block;
		this->block->count++;
            }
        }

	return *this;
    }

    bool operator ==(const StrongPtr<T>& other) const { //true if refer to same
	if(this->block == other.block){
	    return true;
        }
	else{
	    return false;
        }
    }

    bool operator !=(const StrongPtr<T>& other) const {
	if(this->block != other.block){
	    return true;
        }
	else{
	    return false;
        }
    }
};

#endif
