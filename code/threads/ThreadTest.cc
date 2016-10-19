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
    default:
	printf("No test specified.\n");
	break;
    }
}

