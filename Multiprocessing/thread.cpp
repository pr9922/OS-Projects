
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

struct CpuInfo {
    Thread* activeThread_ = nullptr;
    Queue<Thread>* targetQueue = nullptr;
    Thread* nextThread = nullptr;
};

struct ThreadInfo {
    SpinLock readySpin;
    Queue<Thread>* noQ = nullptr;
    Queue<Thread>* readyQ = new Queue<Thread>;
    PerCPU<CpuInfo> cpuInfo;
    uint32_t nextId = 1000;
}; 

static ThreadInfo *threadInfo = nullptr;

void Thread::init(void) {
    threadInfo = new ThreadInfo();
}

static inline CpuInfo* myCpuInfo(void) {
    return &threadInfo->cpuInfo.mine();
}

class StartThread : public Thread {
public:
    StartThread() : Thread() {
	Debug::printf("We created a new startthread in Core %d\n", SMP::me());
	}
    virtual long run() override {
        Debug::panic("should never call StartThread::run");
        return 0;
    }
};

class IdleThread: public Thread {
protected:
    virtual long run(void) override{
	    while(true){
			Thread::yield();
		}
	    return 0;
	}	
	
};

Thread* Thread::current(void) {
    auto info = myCpuInfo();
    auto me = info->activeThread_;
    if (me == nullptr) {
        me = new StartThread();
        me = new IdleThread();
        info->activeThread_ = me;
    } 
    return me;
}

Thread::Thread() : sem(0), stack(nullptr), esp(0), myId( threadInfo->nextId ++){
    Debug::printf("Creating Thread %d in Core %d\n", this->myId, SMP::me());
   // this->start();
    //threadInfo->readyQ->add(this);
}

extern "C" void contextSwitch(uint32_t newEsp, uint32_t *oldEsp);

static void threadSetActive(void) {
    CpuInfo *me = myCpuInfo();
    if (me->targetQueue != nullptr) {
        me->targetQueue->add(me->activeThread_);
        me->targetQueue = nullptr;
    }
    me->activeThread_ = me->nextThread;
    me->nextThread = nullptr;
}

void Thread::threadSwitch(Queue<Thread>* q) {
    CpuInfo *me = myCpuInfo();
    //Debug::printf("CORE %d picking up\n", me);
    me->targetQueue = q;
    //threadInfo->readySpin.lock();
    me->nextThread = threadInfo->readyQ->remove();
    //threadInfo->readySpin.unlock();
   /* if(me->nextThread == nullptr){
	    me->activeThread_ = new IdleThread();
	    me->activeThread_->run();
	    return;	
	} */
    if (me->nextThread == nullptr) {
        //if (q == threadInfo->readyQ) return;
        if(q == threadInfo->readyQ){
		//	me->activeThread_ = new IdleThread();
	      //  me->activeThread_->run();
	        //Thread::yield();
	        //threadSetActive();	
	        return;
		}
        Debug::shutdown();
    }
    
    //threadInfo->readySpin.lock();
    //Debug::printf("CONTEXT SWITCH %d\n", me);
    contextSwitch(me->nextThread->esp,&Thread::current()->esp);
    //threadInfo->readySpin.unlock();
    threadSetActive();
}

void Thread::yield(void) {
	//CpuInfo *me = myCpuInfo();
	//if(threadInfo->readyQ->isEmpty()){
	//    me->activeThread_ = new IdleThread();
	//    me->activeThread_->run();
	//    return;
    //}
    threadSwitch(threadInfo->readyQ);
}


void Thread::exit(long result) {   
    Debug::printf("Exiting Thread %d in CPU %d\n", Thread::current()->myId, SMP::me());
    Thread::current()->isDone = true;
    Thread::current()->result = result;
    Thread::current()->sem.up();
    
    
   // CpuInfo *me = myCpuInfo();
    //if(threadInfo->readyQ->isEmpty()){
//	    me->activeThread_ = new IdleThread();
//	    me->activeThread_->run();
	   // return;
  //  } 
    threadSwitch(threadInfo->noQ); 
}

long Thread::join() {
    Debug::printf("Joining Thread %d in Core %d\n", Thread::current()->myId, SMP::me());
    sem.down();
    sem.up();
    while(!this->isDone){
	   Thread::yield();	
	}
	//TODO: this ain't gonna work, the other thread running
	//doesn't necessarily need to assign this value
    return this->result;
}

void Thread::entry() {
    threadSetActive();
    Thread::current()->isDone = false;
    Thread* me = Thread::current();
    //if(me == nullptr){ return; }
    Thread::exit(me->run());
    Debug::panic("should never get here\n");
}

void Thread::start() {
    Debug::printf("Starting Thread %d on Core %d\n", this->myId, SMP::me());
    stack = new long[STACK_LONGS];
    esp = (uint32_t)(&stack[STACK_LONGS]);
    push((uintptr_t)entry);
    for (int i=0; i<7; i++) push(0); // regs
    threadInfo->readyQ->add(this);
}
