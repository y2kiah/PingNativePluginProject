#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "../utility/common.h"

#ifdef _WIN32
#include "Windows.h"
#endif

void platformPause();

void yieldThread();

void platformSleep(
    unsigned long ms);

u32 platformGetPid();

#endif