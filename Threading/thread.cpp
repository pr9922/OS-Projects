#include "thread.h"
#include "stdint.h"
#include "debug.h"
#include "heap.h"
#include "machine.h"
#include "queue.h"
#include "atomic.h"
#include "init.h"
#include "kernel.h"
#include "smp.h"

static Thread* curr;
static Queue<Thread> readyQueue;
static bool initCalled;
static int numThreads;

void Thread::init(void) { //set up data structures for thead to work together
    if(initCalled == true){
        return;
    }
    initCalled = true;
    Thread* t = new Thread();  //create initial thread
    curr = t;                   //set current thread
    numThreads = 0;
}

Thread* Thread::current(void) { //return current thread
    return curr;
}

//Never let child thread join its parent 

void Thread::runHelper(void){
    Thread* me = Thread::current();
    long result = me->run();
    Thread::exit(result);
    Thread::yield();
}

Thread::Thread() {   //initializze 
    this->isDone = false;    
    numThreads++;                   //Thread is not done/terminated
    this->myId = numThreads;                    //Set id to number of threads
    void* stackLocation = malloc(STACK_LONGS);  //Pointer to location of stack
    this->stack = (long*)stackLocation;         //Add stack to thread
    this->esp = ((uintptr_t)stackLocation + STACK_LONGS - 4);  //add ESP
    this->push((uintptr_t)runHelper); //push helper function and 7 fake values
    this->push(400);
    this->push(401);
    this->push(402);
    this->push(403);
    this->push(404);
    this->push(405);
    this->push(406);
}



void Thread::yield(void) { //Get next thread from ready queue. Place curent on ready Queue  
    Thread* oldThread = Thread::current();
    Thread* newCurr = readyQueue.remove();
    if(newCurr != nullptr){
            curr = newCurr;
        }
    if(!oldThread->isDone){
        readyQueue.add(oldThread);
    }
    contextSwitch(curr->esp, &(oldThread->esp)); 
}

static int currResult;

void Thread::exit(long result) { //called by thread to stop running permanently
    curr->isDone = true;
    free(curr->stack);
    currResult = result;
    numThreads--;
    curr = readyQueue.remove();
}

long Thread::join() {
    while(!this->isDone){
         Thread::yield();
    }
    return currResult;
}

void Thread::start() { //add to running queue+
    this->isDone = false;
    readyQueue.add(this);
}

