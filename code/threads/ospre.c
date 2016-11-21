#include <iostream>
using namespace std;

semaphore mutex = 1, block = 0;
int active = 0, waiting = 0;
bool must_wait = false;

void foo1()
{
	//1. 获得资源前的操作
	P(mutex);
	if(must_wait)
	{
		++waiting;
		V(mutex);
		P(block);
		P(mutex);
		--waiting;
	}
	++active;
	must_wait = active == 3;
	V(mutex);

	//2. 临界区：对获得的资源进行操作

	//3. 获得资源后的操作
	P(mutex);
	--active;
	if(active == 0)
	{
		int n;
		if(waiting < 3)
			n = waiting;
		else
			n = 3;

		while(n > 0)
		{
			V(block);
			--n;
		}

		must_wait = false;
	}
	V(mutex);
}

void foo2()
{
	P(mutex);
	if(must_wait)
	{
		++waiting;
		V(mutex);
		P(block);
		//注意：这里没有active++，
		//是以为被唤醒时active已经增加过了
	}
	else
	{
		++active;
		must_wait = active==3;
		V(mutex);
	}

	/*临界区：对获得资源进行操作*/

	P(mutex);
	--active;
	if(active == 0)
	{
		int n;
		if(waiting < 3)
			n = waiting;
		else
			n = 3;
		waiting -= n;
		active = n;

		while(n > 0)
		{
			V(block);
			--n;
		}
		must_wait = active==3;
	}
	V(mutex);
}

void  foo3()
{
	P(mutex);
	if(must_wait)
	{
		++waiting;
		V(mutex);
		P(block);
		--waiting;
	}

	++active;//和foo2不同
	must_wait = active==3;
	if(waiting>0 && !must_wait)
		V(block);//接力棒，唤醒下一个
	else//正常结束或者
		//最后一棒交还第一棒的锁
		V(mutex);

	/*临界区：对获得资源进行操作*/

	//到此处时一定已经放弃锁。
	//假设93行waiting为假，则waiting必为0，
	//则106行释放锁；
	//假设must_wait为真，则96行释放锁
	P(mutex);
	--active;
	if(active == 0)
		must_wait = false;
	if(waiting>0 && !must_wait)
	//第一棒，mutex还在手中
		V(block);
	else//正常释放
		V(mutex);
}