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

typedef void (*VoidFunctionPtr)(int arg);
typedef void (*VoidNoArgFunctionPtr)();
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

void LazyLoad_inverse(int vpn)
{
	int temp_phy_num = mymap->Find();
	if(temp_phy_num == -1)
	{
		int tmp = 1 << 30;
		Thread *replaceThread;
		TranslationEntry *replacePage;
		for(int i = 0; i < NumPhysPages; ++i)
		{
			if(tmp > machine->inverseTable[i].recent)
			{
				tmp = machine->inverseTable[i].recent;
				temp_phy_num = i;
			}
		}
		replaceThread = (Thread *)(machine->inverseTable[temp_phy_num].owner);
		replacePage = (TranslationEntry *)(machine->inverseTable[temp_phy_num].page);
		if(replacePage->dirty)
		{
			memcpy(&(replaceThread->space->swapFile[replacePage->virtualPage * PageSize]),
				&(machine->mainMemory[temp_phy_num * PageSize]),PageSize);
		}
		if(replaceThread == currentThread)
		{
			for(int i = 0; i < TLBSize; ++i)
			{
				if(machine->tlb[i].valid && machine->tlb[i].virtualPage == replacePage->virtualPage)
				{
					machine->tlb[i].valid = false;
				}
			}
		}
		replacePage->valid = false;
		replacePage->physicalPage = -1;
	}
	machine->inverseTable[temp_phy_num].owner = currentThread;
	machine->inverseTable[temp_phy_num].vpn = vpn;
	machine->inverseTable[temp_phy_num].recent = 0;
	machine->inverseTable[temp_phy_num].page = &(machine->pageTable[vpn]);

	machine->pageTable[vpn].valid = true;
	machine->pageTable[vpn].physicalPage = temp_phy_num;
	machine->pageTable[vpn].virtualPage = vpn;
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
				machine->tlb[i].physicalPage = temp_phy_num;
		}
	}
	memcpy(&(machine->mainMemory[temp_phy_num * PageSize]),
		&(currentThread->space->swapFile[machine->pageTable[vpn].virtualPage * PageSize]),PageSize);
}
int pagefaultnum = 0;

struct spaceAndPC
{
	AddrSpace *space;
	int pc;
};
void forkFunc(int p)
{
	printf("i am here\n");
	spaceAndPC *tmp = (spaceAndPC *)p;
	AddrSpace *space = new AddrSpace(tmp->space);
	currentThread->space = space;

	machine->WriteRegister(PCReg, tmp->pc);
	machine->WriteRegister(NextPCReg,tmp->pc+4);

	currentThread->SaveUserState();

	machine->Run();
}
void execFunc(int p)
{
	char *name = (char *)p;

	OpenFile *executable = fileSystem->Open(name);
    AddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", name);
	return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}
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
		//delete currentThread->space;
		currentThread->Finish();
	}
	else if ((which == SyscallException) && (type == SC_Create))
	{
		int addr = machine->ReadRegister(4);
		char buffer[50];
		int len = 0;
		int tmp;
		while(true)
		{
			machine->ReadMem(addr++, 1, &tmp);
			if(tmp != 0)
			{
				if(tmp == 43 || tmp < 0)
					tmp = 46;
				buffer[len++] = tmp;
			}
			else
			{
				buffer[len] = 0;
				break;
			}
		}
		fileSystem->Create(buffer, 0);
		DEBUG('6',"Create %s\n", buffer);
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Open))
	{
		int addr = machine->ReadRegister(4);
		char buffer[20];
		int len = 0;
		int tmp;
		while(true)
		{
			machine->ReadMem(addr++, 1, &tmp);
			if(tmp != 0)
			{
				if(tmp == 43 || tmp < 0)
					tmp = 46;
				buffer[len++] = tmp;
			}
			else
			{
				buffer[len] = 0;
				break;
			}
		}
		OpenFile *fd = fileSystem->Open(buffer);
		DEBUG('6',"Open %s, fd = %d\n", buffer, (int)fd);
		machine->WriteRegister(2,(int)fd);
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Close))
	{
		int fd = machine->ReadRegister(4);
		
		fileSystem->CloseDIY((OpenFile *)fd);
		DEBUG('6',"Close %d\n", fd);
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Read))
	{
		int addr = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		OpenFile *fd = (OpenFile *)(machine->ReadRegister(6));
		
		char buffer[20];
		int readSize = fd->Read(buffer, size);
		for(int i = 0; i < readSize; ++i)
			machine->WriteMem(addr++, 1, buffer[i]);
		machine->WriteRegister(2, readSize);

		machine->Refresh();
		DEBUG('6',"Read %d bytes, contents are %x\n", readSize, *((int *)buffer));		
	}
	else if ((which == SyscallException) && (type == SC_Write))
	{
		int addr = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		OpenFile *fd = (OpenFile *)(machine->ReadRegister(6));
		
		int tmp;
		char buffer[20];
		for(int i = 0; i < size; ++i)
		{
			machine->ReadMem(addr++, 1, &tmp);
			buffer[i] = tmp;
		}
		DEBUG('6',"Write, contents are %x, size is %d\n", *((int *)buffer), size);

		fd->Write(buffer, size);

		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_GYS))
	{
		printf("lucky dog.\n");
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Yield))
	{
		DEBUG('6',"Thread yeild.\n");
		machine->Refresh();
		currentThread->Yield();
	}
	else if ((which == SyscallException) && (type == SC_Fork))
	{
		DEBUG('6',"Thread fork.\n");
		spaceAndPC *tmp = new spaceAndPC;
		tmp->pc = machine->ReadRegister(4);
		tmp->space = currentThread->space;
		Thread *t1 = taskmanager->createThread("child",3);
		currentThread->addChild(t1);
		t1->addFather(currentThread);
		t1->Fork(forkFunc, (int)tmp);
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Exec))
	{
		DEBUG('6', "Thread exec.\n");
		int addr = machine->ReadRegister(4);
		char *buffer = new char[20];
		int len = 0;
		int tmp;
		while(true)
		{
			machine->ReadMem(addr++, 1, &tmp);
			if(tmp != 0)
			{
				if(tmp == 43 || tmp < 0)
					tmp = 46;
				buffer[len++] = tmp;
			}
			else
			{
				buffer[len] = 0;
				break;
			}
		}

		Thread *t1 = taskmanager->createThread("childExec",3);
		currentThread->addChild(t1);
		t1->addFather(currentThread);
		machine->WriteRegister(2, (int)t1);

		t1->Fork(execFunc, (int)buffer);
		
		machine->Refresh();
	}
	else if ((which == SyscallException) && (type == SC_Join))
	{
		Thread *tmp = (Thread *)machine->ReadRegister(4);
		int count = 0;
		for(int i = 0; i < 10; ++i)
		{
			if(currentThread->childThread[i] == tmp)
			{
				count = i;
				break;
			}
		}
		while(currentThread->childThread[count] != NULL)
		{
			DEBUG('6', "thread %s have to wait.\n", currentThread->getName());
			currentThread->Yield();
		}
		machine->Refresh();
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
			//LazyLoad(vpn);
			LazyLoad_inverse((int)vpn);
		}
    }
    else 
    {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}
