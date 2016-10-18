#ifndef THREADMANAGE_H
#define THREADMANAGE_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

#define max_task 128
class taskManager
{
private:
	bool notepad[max_task];
	int tcounter;
public:
	taskManager();
	//~taskManager();
	Thread* createThread(char* name, int pri, int Uid=0);
	void deleteThread(Thread* target);
	int newTid();
	void printThread();
};
#endif