#include "build_config.h"
#include "platform/platform.h"
#include "platform/ping.h"
#include <iostream>

#include "platform/platform.cpp"
#include "platform/timer.cpp"
#include "platform/ping.cpp"


int main(int argc, char *argv[])
{
	Ping pings[5] = {
        ping(
			"192.168.0.185", // host
			10,				 // number of requests in sequence
			64U,			 // data size
			30,				 // ttl
			1000U),			 // timeout ms
        ping("google.com", 10),
        ping("yahoo.com", 10),
        //ping("127.0.0.1"), // Note: adding localhost to the mix seems to invalidate other sockets
        ping("gamedev.net", 10),
        ping("unity3d.com", 10)
    };

    for(;;) {
    	u32 finishedCount = 0;
		
        for (auto& ping : pings) {
			if (pollResult(ping)) {
				++finishedCount;
			}
        }

		if (finishedCount == countof(pings)) {
			break;
		}
    }

	platformPause();

	return 0;
}
