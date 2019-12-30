#ifdef _WIN32

#include "ping.h"


/**
 * @param host  can be a dotted-quad IP address or a host name
 * @returns 0 on success, -1 on error
 */
static
s32
resolveDestinationHost(
    const char* host,
    sockaddr_in& dest)
{
    // find if it's a dotted IP address
    u32 address = inet_addr(host);
    if (address != INADDR_NONE)
    {
        dest.sin_addr.s_addr = address;
        dest.sin_family = AF_INET;
    }
    else {
        hostent* hp = gethostbyname(host);
        if (hp) {
            // Found an address for that host, so save it
            memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length);
            dest.sin_family = hp->h_addrtype;
        }
        else {
            // Not a recognized hostname either!
            printf("Failed to resolve %s\n", host);
            return Result_Error;
        }
    }

    return Result_Success;
}

/**
 * @param ttl  number of hops
 * @returns 0 on success, -1 on error
 */
s32
createSocket(
    u8 ttl,
    SOCKET& outSocket)
{
    outSocket = socket(
        AF_INET,
        SOCK_RAW,
        IPPROTO_ICMP);
        
    if (outSocket == INVALID_SOCKET) {
        printf("Failed to create raw socket: %d\n", WSAGetLastError());
        return Result_Error;
    }

    s32 opt = setsockopt(
        outSocket,
        IPPROTO_IP,
        IP_TTL,
        (const char*)&ttl, 
        sizeof(ttl));

    if (opt == SOCKET_ERROR) {
        printf("TTL setsockopt failed: %d\n", WSAGetLastError());
        return Result_Error;
    }

    u_long nonBlockingMode = 1;
    opt = ioctlsocket(outSocket, FIONBIO, &nonBlockingMode);
    if (opt != NO_ERROR) {
        printf("ioctlsocket failed with error: %ud\n", opt);
    }

    return Result_Success;
}

void
platform_closesocket(
    SOCKET socket)
{
    closesocket(socket);
}


/**
 * @param packetSize  total size of packet to send including ICMPHeader
 * @returns 0 on success, -1 on error, 2 on pending
 */
s32
sendPingPacket(
    SOCKET socket,
    const sockaddr_in& dest,
    const u8* buffer,
    u32 packetSize)
{
    s32 bytes = sendto(
        socket,
        (const char*)buffer,
        packetSize,
        0, 
        (sockaddr*)&dest,
        sizeof(dest));
    
    if (bytes == SOCKET_ERROR) {
        s32 err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return Result_Pending;
        }
        else {
            printf("Failed to send: %d\n", err);
            return Result_Error;
        }
    }

    printf(
        "Pinging %s with %d bytes of data:\n",
        inet_ntoa(dest.sin_addr),
        (s32)(bytes - sizeof(ICMPHeader)));

    return Result_Success;
}

/**
 * @param recvBuffer  buffer to receive data, must be larger than
 *  request buffer + sizeof(ICMPHeader) due to IP header options
 * @param bufferSize  size of the buffer pointed to by recvBuffer
 * @returns 0 on success, -1 on error, 2 on pending
 */
s32
getPingReply(
    SOCKET socket,
    u8* recvBuffer,
    u32 bufferSize,
    sockaddr_in& source)
{
    s32 fromLen = sizeof(source);

    s32 bytes = recvfrom(
        socket,
        (char*)recvBuffer, 
        bufferSize,
        0,
        (sockaddr*)&source,
        &fromLen);

    if (bytes == SOCKET_ERROR) {
        s32 err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return Result_Pending;
        }
        else {
            printf("Failed to read reply: %d\n", err);
            return Result_Error;
        }
    }
    else if (bytes == 0) {
        printf("Connection closed\n");
        return Result_Error;
    }

    return Result_Success;
}


static atomic_lock running = ATOMIC_FLAG_INIT;
static HANDLE hThread = 0;
static DWORD threadId = 0;


DWORD WINAPI
pingJobProcess(
    LPVOID lpParam)
{
    PingJobHnd runningJobs[MaxPingJobs]{};
    u32 numRunning = 0;

    initHighPerfTimer();

    // start Winsock
    // TODO: replace with platform agnostic "platform_startupSockets" call
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Failed to find Winsock 2.2.\n");
        return 1;
    }

    for (;;)
	{
        if (numRunning == 0)
        {
            // there are no running jobs, use wait_pop to yield the thread until we get a new job
            PingJobHnd hnd = null_h32;
            if (!jobQueue.wait_pop(&hnd, 1000))
            {
                // wait timed out, end the thread
                break;
            }

            // exit thread when a null handle is pushed onto the queue
            if (hnd == null_h32) {
                break;
            }
            // otherwise add this job to the running list
            runningJobs[numRunning++] = hnd;
        }
        else {
            {
                PingJobHnd hnd = null_h32;
                if (jobQueue.try_pop(&hnd))
                {
                    // exit thread when a null handle is pushed onto the queue
                    if (hnd == null_h32) {
                        break;
                    }
                    // otherwise add this job to the running list
                    assert(numRunning < MaxPingJobs && "too many jobs");
                    runningJobs[numRunning++] = hnd;
                }
            }

            // now iterate the running job list, each job is a state machine and is non-blocking
            for(u32 j = 0;
                j < numRunning;
                ++j)
            {
                PingJobHnd hnd = runningJobs[j];
                PingJob* job = jobs[hnd];

                if (job) {
                    SequenceStatus status =
                        runPingSequence(*job);
                    
                    if (status != Sequence_Running)
                    {
                        // sequence finished, remove from running jobs by swap and pop
						runningJobs[j] = runningJobs[--numRunning];
                    }
                }
                else {
                    // invalid handle, error
                }
            }
        }
	}

    WSACleanup();
    running.clear();
    hThread = 0;
    threadId = 0;

	return 0;
}


s32
startPingJobThread()
{
    if (!running.test_and_set())
    {
        hThread = CreateThread( 
            NULL,             // default security attributes
            0,                // use default stack size  
            pingJobProcess,   // thread function name
            nullptr,          // argument to thread function 
            0,                // use default creation flags 
            &threadId);       // returns the thread identifier 
    }

    return Result_Success;
}

#endif
