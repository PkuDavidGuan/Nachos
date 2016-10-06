#include "taskManager.h"
#include "system.h"

taskManager::taskManager()
{
	tcounter = 0;
	for(int i = 0; i < max_task; ++i)
		notepad[i] = false;
}

Thread*
taskManager::createThread(char* name, int Uid)
{
	int num = newTid();
	if(num != -1)
		return (new Thread(name, num, Uid));
	else
		return NULL;
}

void
taskManager::deleteThread(Thread* target)
{
	tcounter--;
	notepad[target->getTid()] = false;
	delete target;
}

int 
taskManager::newTid()
{
	if(tcounter < max_task)
	{
		tcounter++;
		for(int i = 0; i < max_task; ++i)
		{
			if(notepad[i] == false)
			{
				notepad[i] = true;
				return i;
			}
		}
	}
	else
		return -1;
}

void 
taskManager::printThread()
{
	printf("NAME       TID       UID       STATUS\n");
	printf("-----------Current thread------------\n");
	currentThread->Print();
	scheduler->Print();
}