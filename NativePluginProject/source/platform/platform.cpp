#include <atomic>
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


// NOT _WIN32
#else

#include "../../vendor/SDL2/SDL_messagebox.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <ctime>

void platformPause()
{
	pause();
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

// END NOT WIN32
#endif
