#include "platform.h"

#ifdef _WIN32

void platformPause()
{
	system("pause");
}

void yieldThread()
{
	SwitchToThread();
}

void platformSleep(
	unsigned long ms)
{
	Sleep(ms);
}

u32 platformGetPid()
{
	return (u32)GetCurrentProcessId();
}


// NOT _WIN32
#else

#include <unistd.h>


void platformPause()
{
	//pause();
}

#ifdef _SC_PRIORITY_SCHEDULING
#include <sched.h>

void yieldThread()
{
	sched_yield();
}

#else

#include <thread>
void yieldThread()
{
	std::this_thread::yield();
}

#endif

void platformSleep(
	unsigned long ms)
{
	usleep(ms * 1000);
}

u32 platformGetPid()
{
	return (u32)getpid();
}


// END NOT WIN32
#endif
