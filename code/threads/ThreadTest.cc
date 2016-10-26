// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 4;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 3; num++) {
	printf("*** thread %d looped %d times\n", which, num);
            currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = taskmanager->createThread("forked thread", 1);

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}
void
TS(int none)
{
    taskmanager->printThread();
}
//----------------------------------------------------------------------
//implement the TS function
//----------------------------------------------------------------------
void
ThreadTest2()
{
    Thread *t1 = taskmanager->createThread("Alice",1);
    t1->Fork(SimpleThread, 1);

    Thread *t2 = taskmanager->createThread("David",2);
    t2->Fork(SimpleThread, 2);

    Thread *t3 = taskmanager->createThread("Carol",3);
    t3->Fork(SimpleThread, 3);

    Thread *t4 = taskmanager->createThread("print task",4);
    t4->Fork(TS, 0);
}

void printA(int none)
{
    for(int i = 0; i < 10; ++i)
        printf("A is RUNNING\n");
}
void printB(int none)
{
    for(int j = 0; j < 10; ++j)
        printf("B is RUNNING\n");
}
void printC(int none)
{
        printf("C is RUNNING\n");
}

//----------------------------------------------------------------------
//Test HPF
//----------------------------------------------------------------------
void ThreadTest3()
{
    Thread *t1 = taskmanager->createThread("Alice",3);
    t1->Fork(printA, 1);

    Thread *t2 = taskmanager->createThread("David",2);
    t2->Fork(printB, 2);

    Thread *t3 = taskmanager->createThread("Carol",1);
    t3->Fork(printC, 3);
}

void simulateTime(int when)
{
    for(int i = 0; i < when; ++i)
    {
        interrupt->OneTick();
    }
}
//----------------------------------------------------------------------
//implement the time-slice and multileval feedback queue dispatch
//----------------------------------------------------------------------
void ThreadTest4()
{
    Thread *t1 = taskmanager->createThread("foo",1);
    t1->Fork(simulateTime, 100);
    Thread *t2 = taskmanager->createThread("bar",1);
    t2->Fork(simulateTime, 100);
}

int buffer = 0;
Semaphore* empty = new Semaphore("empty",5);
Semaphore* full = new Semaphore("full", 0);
Semaphore* Mutex = new Semaphore("mutex", 1);
void  Produce(int count)
{
    for(int i = 0; i < count; ++i)
    {
        empty->P();
        Mutex->P();
        buffer++;
        printf("%s produced an item into the buffer, %d left.\n", 
            currentThread->getName(), buffer);
        Mutex->V();
        full->V();
        
    }
}
void Consume(int count)
{
    for(int i = 0; i < count; ++i)
    {
        full->P();
        Mutex->P();
        buffer--;
        printf("%s consumed an item from the buffer, %d left.\n", 
            currentThread->getName(), buffer);
        Mutex->V();
        empty->V();
        
    }
}
//----------------------------------------------------------------------
//Test producer-consumer problem(using semaphore)
//----------------------------------------------------------------------
void ThreadTest5()
{
    Thread *t1 = taskmanager->createThread("consumer1", 1);
    t1->Fork(Consume, 3);
    Thread *t2 = taskmanager->createThread("producer1", 1);
    t2->Fork(Produce, 3);
    Thread *t3 = taskmanager->createThread("producer2", 1);
    t3->Fork(Produce, 3);
    Thread *t4 = taskmanager->createThread("producer3",1);
    t4->Fork(Produce, 3);
    Thread *t5 = taskmanager->createThread("consumer2",1);
    t5->Fork(Consume, 3);
    Thread *t6 = taskmanager->createThread("consumer3",1);
    t6->Fork(Consume, 3);
}

int reading = 0;
int writing = 0;
Condition* rok = new Condition("read ok");
Condition* wok = new Condition("write ok");
Lock* MUTEX = new Lock("mutex");
void reader(int count)
{
    for(int i = 0; i < count; ++i)
    {
        MUTEX->Acquire();
        reading++;
        if(writing > 0)
            rok->Wait(MUTEX);
        rok->Signal(MUTEX);
        MUTEX->Release();

        printf("%s is reading.\n", currentThread->getName());

        MUTEX->Acquire();
        reading--;
        if(reading == 0)
            wok->Signal(MUTEX);
        MUTEX->Release();
    }
}
void writer(int count)
{
    for(int i = 0; i < count; ++i)
    {
        MUTEX->Acquire();
        if(writing > 0 || reading > 0)
            wok->Signal(MUTEX);
        writing++;
        MUTEX->Release();

        printf("%s is writing.\n", currentThread->getName());

        MUTEX->Acquire();
        writing--;
        if(reading > 0)
            rok->Signal(MUTEX);
        else
            wok->Signal(MUTEX);
        MUTEX->Release();
    }
}
//----------------------------------------------------------------
//Test the first type of reader-writer problem(using lock 
//and condition variable)
//----------------------------------------------------------------
void ThreadTest6()
{
    Thread *t1 = taskmanager->createThread("reader1", 1);
    t1->Fork(reader, 3);
    Thread *t2 = taskmanager->createThread("writer1", 1);
    t2->Fork(writer, 3);
    Thread *t3 = taskmanager->createThread("reader2", 1);
    t3->Fork(reader, 3);
    Thread *t4 = taskmanager->createThread("reader3",1);
    t4->Fork(reader, 3);
    Thread *t5 = taskmanager->createThread("writer2",1);
    t5->Fork(writer, 3);
    Thread *t6 = taskmanager->createThread("writer3",1);
    t6->Fork(writer, 3);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    case 4:
    ThreadTest4();
    break;
    case 5:
    ThreadTest5();
    break;
    case 6:
    ThreadTest6();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

