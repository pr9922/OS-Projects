#include "semaphore.h"
#include "thread.h"
#include "debug.h"


Semaphore::Semaphore(const uint32_t count) {
    this->counter = count;
}

void Semaphore::down() {
    spin.lock();
    while(counter == 0){
	    spin.unlock();
	    //semQueue.add(Thread::current());
	    Thread::yield();
	    spin.lock();
    }
    //else{
       counter--;
        //spin.unlock();
     //}
	//counter--;
	spin.unlock();
}

void Semaphore::up() {
    spin.lock();
    counter++;
    //Thread::addToQueue(semQueue.remove());
    spin.unlock();
} 

