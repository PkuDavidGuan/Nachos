// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void tlb_miss(unsigned int vpn)
{
    int num = 0;
    int temp;
    bool freeTLB = false;

    for(int i = 0; i < TLBSize; ++i)
	{
    	if(machine->tlb[i].valid == false)
    	{
    		num = i;
			freeTLB = true;
				//printf("Found a void tlb entry: %d\n", i);
    		break;
    	}
    }

    if(freeTLB == false)
    {
		temp = 1 << 30;
	    	/*LFU*/
	    	// temp = 1 << 30;
	    	// for(int i = 0; i < TLBSize; ++i)
	    	// {
			// 	//printf("tlb %d frequency: %d\n", i, machine->tlb[i].frequency);
	    	// 	if(temp > machine->tlb[i].frequency)
	    	// 	{
	    	// 		temp = machine->tlb[i].frequency;
	    	// 		num = i;
	    	// 	}
			// 	machine->tlb[i].frequency = 0;
	    	// }

	    	/*LRU*/
		for(int i = 0; i < TLBSize; ++i)
	    {
				//printf("tlb %d recent: %d\n", i, machine->tlb[i].recent);
	    	if(temp > machine->tlb[i].recent)
	    	{
	    		temp = machine->tlb[i].recent;
	    		num = i;
	    	}
	    }
	  		//printf("A old tlb entry out: %d\n", num);    	
    }
    machine->tlb[num].valid = true;
    machine->tlb[num].virtualPage = machine->pageTable[vpn].virtualPage;
    machine->tlb[num].physicalPage = machine->pageTable[vpn].physicalPage;
    machine->tlb[num].readOnly = machine->pageTable[vpn].readOnly;
    machine->tlb[num].use = machine->pageTable[vpn].use;
    machine->tlb[num].dirty = machine->pageTable[vpn].dirty;
	machine->tlb[num].frequency = 0; //System doesn't use the page until the translate was called again
    machine->tlb[num].recent = tlbcounter->count;
	machine->tlb[num].buddy = &(machine->pageTable[vpn]);
}

void LazyLoad(unsigned int vpn)
{
	int temp_phy_num = mymap->Find();
	if(temp_phy_num >= 0)
	{
		machine->pageTable[vpn].physicalPage = temp_phy_num;
	}
	else
	{
		//find a old page
		int tmp = 1 << 30;
		int oldpage = 0;

		for(int i = 0; i < currentThread->space->GetPageNum(); ++i)
		{
			if(machine->pageTable[i].valid)
			{
				ASSERT(machine->pageTable[i].physicalPage>=0);
				if(tmp > machine->pageTable[i].recent)
				{
					tmp = machine->pageTable[i].recent;
					oldpage = i;
				}
			}
		}

		if(machine->pageTable[oldpage].dirty)
		{
			memcpy(&(currentThread->space->swapFile[machine->pageTable[oldpage].virtualPage * PageSize]),
				&(machine->mainMemory[machine->pageTable[oldpage].physicalPage * PageSize]),PageSize);
		}

		if(machine->tlb != NULL)
		{
			for(int i = 0; i < TLBSize; ++i)
			{
				if(machine->tlb[i].valid && machine->tlb[i].virtualPage == machine->pageTable[oldpage].virtualPage)
				{
					machine->tlb[i].valid = false;
					break;
				}
			}
		}
		machine->pageTable[oldpage].valid = false;
		machine->pageTable[vpn].physicalPage = machine->pageTable[oldpage].physicalPage;
		temp_phy_num = machine->pageTable[oldpage].physicalPage;
	}
	ASSERT(temp_phy_num >= 0);
	machine->pageTable[vpn].valid = true;
	machine->pageTable[vpn].frequency = 0;
	machine->pageTable[vpn].recent = 0;
	machine->pageTable[vpn].use = false;
	machine->pageTable[vpn].dirty = false;
	machine->pageTable[vpn].readOnly = false;

	if(machine->tlb != NULL)
	{
		for(int i = 0; i < TLBSize; ++i)
		{
			if(machine->tlb[i].valid && machine->tlb[i].virtualPage == vpn)
			{
				machine->tlb[i].physicalPage = temp_phy_num;
				break;
			}
		}
	}

	memcpy(&(machine->mainMemory[temp_phy_num * PageSize]),
			&(currentThread->space->swapFile[machine->pageTable[vpn].virtualPage * PageSize]),PageSize);
}
int pagefaultnum = 0;
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) 
    {
		DEBUG('a', "Shutdown, initiated by user program.\n");
   		interrupt->Halt();
    }
	else if ((which == SyscallException) && (type == SC_Exit))
	{
		printf("Thread %s exit.\n", currentThread->getName());
		printf("Exit num: %d\n", machine->ReadRegister(BadVAddrReg));
		printf("----------------------\n");
		printf("Page fault num: %d\n", pagefaultnum);
		printf("----------------------\n");
		mymap->Print();
		printf("----------------------\n");
		printf("\n");
		//interrupt->Halt();
		delete currentThread->space;
		currentThread->Finish();
	}
    else if(which == PageFaultException)      //pagefault
    {
		unsigned int vpn;
		int addr = machine->ReadRegister(BadVAddrReg);
		vpn = (unsigned)addr / PageSize;

		pagefaultnum += 1;

    	if(machine->tlb != NULL)
		{
			tlb_miss(vpn);
		}

		//the real page fault
		if(machine->pageTable[vpn].valid == false)
		{
			LazyLoad(vpn);
		}
    }
    else 
    {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}
