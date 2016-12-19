// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value <= 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{
    name = debugName;
    lock = new Semaphore("lock", 1);
    ownerThread = NULL;
}

Lock::~Lock() 
{
    delete lock;
}

bool Lock::isHeldByCurrentThread()
{
    return ownerThread == currentThread;
}
void Lock::Acquire() 
{
    lock->P();
    ownerThread = currentThread;
}

void Lock::Release()
{
    ASSERT(isHeldByCurrentThread());
    ownerThread = NULL;
    lock->V();
}

//  Wait() -- release the lock, relinquish the CPU until signaled, 
//      then re-acquire the lock
//
//  Signal() -- wake up a thread, if there are any waiting on 
//      the condition
//
//  Broadcast() -- wake up all threads waiting on the condition
Condition::Condition(char* debugName) 
{
    name = debugName;
    waitingList = new List;
}

Condition::~Condition() 
{
    delete waitingList;
}

void Condition::Wait(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    
    ASSERT(conditionLock->isHeldByCurrentThread());
    conditionLock->Release();
    waitingList->Append((void *)currentThread);
    currentThread->Sleep();
    conditionLock->Acquire();
    
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts

    //ASSERT(!isHeldByCurrentThread());
}

int Condition::Signal(Lock* conditionLock) 
{
    int ret = false;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    ASSERT(conditionLock->isHeldByCurrentThread());
    
    Thread* ToRun = (Thread *)waitingList->Remove();
    if(ToRun != NULL)
    {
        scheduler->ReadyToRun(ToRun);
        ret = true;
    }

    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts 
    return ret;
}

void Condition::Broadcast(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    while(!waitingList->IsEmpty())
    {
        ASSERT(conditionLock->isHeldByCurrentThread());
        scheduler->ReadyToRun((Thread *)waitingList->Remove());
    }
    
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts 
}


rwLock::rwLock(char* debugName) 
{
    name = debugName;
    m = new Semaphore("read/write mutex", 1);
    w = new Semaphore("write is ok", 1);
    rc = 0;
}
rwLock::rwLock() 
{
    m = new Semaphore("read/write mutex", 1);
    w = new Semaphore("write is ok", 1);
    rc = 0;
}

rwLock::~rwLock() 
{
    delete m;
    delete w;
}

void rwLock::start_r()
{
    DEBUG('f', "start to read\n");
    m->P();
    rc++;
    if(rc == 1)
        w->P();
    m->V();
}

void rwLock::finish_r()
{
    DEBUG('f',"read finished\n");
    m->P();
    rc--;
    if(rc == 0)
        w->V();
    m->V();
}

void rwLock::start_w()
{
    DEBUG('f',"start to write\n");
    w->P();
}

void rwLock::finish_w()
{
    DEBUG('f', "write finished\n");
    w->V();
}

#ifdef FILESYS
Pipe::Pipe()
{
    memset(buffer, 0, sizeof(buffer));
    readPos = putPos = 0;
    lineReady = false;
    m = new Semaphore("pipe", 1);
    full = new Semaphore("full", PipeSize);
    empty = new Semaphore("empty", 0);
    line = new Semaphore("read a line", 0);
}
Pipe::~Pipe()
{
    delete m;
    delete full;
    delete empty;
    delete line;
}
void Pipe::Put(char ch)
{
    full->P();
    m->P();
    buffer[putPos] = ch;
    putPos += 1;
    putPos %= PipeSize;
    m->V();
    empty->V();
    if(ch == '\n')
    {
        lineReady = true;
        line->P();
    }
}
char Pipe::Get()
{
    char ret;
    empty->P();
    m->P();
    ret = buffer[readPos];
    readPos += 1;
    readPos %= PipeSize;
    m->V();
    full->V();
    if(readPos == putPos && lineReady)
    {
        lineReady = false;
        line->V();
    }
    return ret;
}
#endif