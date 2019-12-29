#include "ping.h"
#include "timer.h"
#include "../utility/sparse_handle_map_16.h"
#include "../utility/concurrent_queue.h"


#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>


struct PingJob {
    PingSequence   sequence;
    SOCKET         socket;
    sockaddr_in    destAddr;
    sockaddr_in    sourceAddr;
    u8             sendBuffer[MaxPacketSize];
    u8             receiveBuffer[ReceiveBufferSize];
};


SparseHandleMap16_Typed_WithBuffer(
	PingJob,
	PingJobMap,
	PingJobHnd,
	0,
	MaxPingJobs);

ConcurrentQueue_Typed_WithBuffer(
	PingJobHnd,
	PingJobQueue,
	MaxPingJobs,
	0);

static PingJobMap   jobs;
static PingJobQueue jobQueue;

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
static
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


/**
 * Make a ping request, fill the data section with 4 bytes of request timestamp, then hex "dada"
 */
static
void
makePingPacket(
    u8* buffer,
    u16 packetSize,
    u16 seq,
    ICMPHeader& outHdr)
{
    memset(buffer, 0, packetSize);

    ICMPHeader& hdr = *(ICMPHeader*)buffer;
    hdr.message = ICMP_EchoRequest;
    hdr.id      = (u16)GetCurrentProcessId();
    hdr.seq     = htons(seq);

    memset(
        buffer + sizeof(ICMPHeader),
        0xDA,
        packetSize - sizeof(ICMPHeader));

    hdr.checksum = checksum((u16*)buffer, packetSize);

    outHdr = hdr;
}


/**
 * @param packetSize  total size of packet to send including ICMPHeader
 * @returns 0 on success, -1 on error, 2 on pending
 */
static
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
static
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


/**
 * @returns 0 on success, -1 on error, 1 on ignore, 2 on pending
 */
static
s32
handleReply(
    PingRequest& req,
    u16 forSeq,
    u8* buffer,
    u32 bytes,
    const sockaddr_in& from)
{
    IPHeader* reply = (IPHeader*)buffer;

    // skip to the ICMPHeader within the packet
    u16 headerLen = reply->headerLen * sizeof(u32);
    ICMPHeader& pingReply = *(ICMPHeader*)((char*)reply + headerLen);

    // do some error checking on the reply
    if (bytes < headerLen + sizeof(ICMPHeader))
    {
        printf("Too few bytes from %s\n", inet_ntoa(from.sin_addr));
        return Result_Error;
    }
    else if (pingReply.type != ICMPType_EchoReply
             && pingReply.type != ICMPType_TimeExceeded)
    {
        printf(controlMessageString(pingReply.message));
        printf("\n");
        return Result_Error;
    }
    else if (pingReply.id != (u16)GetCurrentProcessId()) {
        // must be a reply for another pinger running locally, ignore it
        return Result_Ignore;
    }

    u16 replySeq = ntohs(pingReply.seq);
    if (replySeq != forSeq) {
        printf("Bad sequence number %d, expected %d\n", replySeq, forSeq);
        return Result_Error;
    }

    // calculate number of hops
    s32 nHops = 256 - reply->ttl;
    // TTL came back 64, so ping was probably to a host on the LAN, single hop.
    if (nHops == 192) {
        nHops = 1;
    }
    // probably localhost
    else if (nHops == 128) {
        nHops = 0;
    }

    req.replyTime = timer_queryCounts();
    req.elapsedMS = (r32)timer_millisBetween(req.sendTime, req.replyTime);

    u16 totalLen = ntohs(reply->totalLen);
    u16 dataBytes = totalLen - headerLen - sizeof(ICMPHeader);

    if (pingReply.type != ICMPType_TimeExceeded)
    {
        printf(
            "Reply from %s: bytes=%d seq=%d/%d hops=%d time=%.1fms TTL=%d\n",
            inet_ntoa(from.sin_addr),
            dataBytes,
            replySeq,
            pingReply.seq,
            nHops,
            req.elapsedMS,
            reply->ttl);
    }
    else {
        printf(
            "Reply from %s: bytes=%d seq=%d/%d, TTL Expired.\n",
            inet_ntoa(from.sin_addr),
            dataBytes,
            replySeq,
            pingReply.seq);
    }

    return Result_Success;
}

#else
// Linux

#endif

static
void
calcStats(
    PingSequence& sequence)
{
    if (sequence.stats.received > 0)
    {
        r32 totalRoundTrip = 0.f;
        r32 maxRoundTrip = 0.f;
        r32 minRoundTrip = FLT_MAX;

        for(u32 r = 0;
            r < sequence.seq;
            ++r)
        {
            PingRequest& req = sequence.requests[r];
            if (req.status == Ping_Received) {
                maxRoundTrip = max(maxRoundTrip, req.elapsedMS);
                minRoundTrip = min(minRoundTrip, req.elapsedMS);
                totalRoundTrip += req.elapsedMS;
            }
        }

        sequence.stats.maxRoundTrip = maxRoundTrip;
        sequence.stats.minRoundTrip = minRoundTrip;
        sequence.stats.avgRoundTrip = totalRoundTrip / (r32)sequence.stats.received;

        r32 totalVariance = 0.f;

        for(u32 r = 0;
            r < sequence.seq;
            ++r)
        {
            PingRequest& req = sequence.requests[r];
            if (req.status == Ping_Received) {
                r32 deviation = req.elapsedMS - sequence.stats.avgRoundTrip;
                totalVariance += (deviation * deviation);
            }
        }

        sequence.stats.stdDevRoundTrip = sqrtf(totalVariance / (r32)sequence.stats.received);
    }

    sequence.stats.pctLost = (r32)sequence.stats.lost / (r32)sequence.stats.sent;
}

static
SequenceStatus
runPingSequence(
    PingJob& job)
{
    u32 dataSize = job.sequence.dataSize;
    u8 ttl = job.sequence.ttl;

    SequenceStatus status = (SequenceStatus)job.sequence.status.load(std::memory_order_relaxed);

    // sequence is inactive and ready to run, set up the socket
    if (status == Sequence_Inactive)
    {
        if (ok(resolveDestinationHost(job.sequence.host, job.destAddr)) &&
            ok(createSocket(ttl, job.socket)))
        {
            status = Sequence_Running;
        }
        else {
            status = Sequence_Error;
        }
    }
    // socket is ready, send the sequence of ping requests
    if (status == Sequence_Running)
    {
        // for the current request in the sequence...
        PingRequest& req = job.sequence.requests[job.sequence.seq];

        u16 packetSize = min((u16)(sizeof(ICMPHeader) + job.sequence.dataSize), (u16)MaxPacketSize);
        
        // send the ICMP echo request
        if (req.status == Ping_Inactive)
        {
            makePingPacket(
                job.sendBuffer,
                packetSize,
                job.sequence.seq,
                req.requestHdr);

            memset(&req.replyHdr, 0, sizeof(ICMPHeader));

            req.status = Ping_Requested;
        }
        
        if (req.status == Ping_Requested)
        {
            s32 result = sendPingPacket(
                job.socket,
                job.destAddr,
                job.sendBuffer,
                packetSize);
            
            if (result == Result_Success) {
                req.sendTime = timer_queryCounts();
                ++job.sequence.stats.sent;
                req.status = Ping_WaitingForReply;
            }
            else if (result == Result_Error) {
                req.status = Ping_Error;
                status = Sequence_Error;
            }
        }

        if (req.status == Ping_WaitingForReply)
        {
            s32 result = getPingReply(
                job.socket,
                job.receiveBuffer,
                ReceiveBufferSize,
                job.sourceAddr);

            if (result == Result_Success)
            {
                result = handleReply(
                    req,
                    job.sequence.seq,
                    job.receiveBuffer,
                    packetSize,
                    job.sourceAddr);
                
                if (result == Result_Success)
                {
                    req.status = Ping_Received;

                    ++job.sequence.seq;
                    ++job.sequence.stats.received;
                    calcStats(job.sequence);
                    
                    if (job.sequence.seq == job.sequence.numRequests) {
                        status = Sequence_Finished;
                    }
                }
            }

            if (result == Result_Error) {
                req.status = Ping_Error;
                status = Sequence_Error;
            }

            // request timed out
            if (result == Result_Pending
                && job.sequence.timeoutMS > 0
                && (timer_queryMillisSince(req.sendTime) >= job.sequence.timeoutMS))
            {
                req.status = Ping_TimedOut;
                
                ++job.sequence.seq;
                ++job.sequence.stats.lost;
                calcStats(job.sequence);
                
                if (job.sequence.seq == job.sequence.numRequests) {
                    status = Sequence_Finished;
                }

            }
        }

        if (status > Sequence_Running) {
            closesocket(job.socket);
        }
    }

    job.sequence.status.store(status, std::memory_order_release);
    return status;
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

static
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


Ping
ping(
    const char* host,
    u16 numRequests,
    u16 dataSize,
    u8  ttl,
    u16 timeoutMS,
    u16 intervalMS)
{
    Ping p{ null_h32, Sequence_Inactive, {} };

    PingJob* pJob = nullptr;
    p.hnd = jobs.insert(nullptr, &pJob);

    if (p.hnd != null_h32)
    {
        PingSequence& sequence = pJob->sequence;

        sequence.host = host;
        sequence.dataSize = dataSize;
        sequence.numRequests = numRequests;
        sequence.timeoutMS = timeoutMS;
        sequence.intervalMS = intervalMS;
        sequence.ttl = ttl;

        jobQueue.push(p.hnd);
        
        startPingJobThread();
    }

    return p;
}

bool
pollResult(
    Ping& ping)
{
    if (ping.hnd != null_h32)
    {
        PingJob* job = jobs[ping.hnd];
        if (job)
        {
            ping.status = (SequenceStatus)job->sequence.status.load(std::memory_order_acquire);

            if (ping.status == Sequence_Finished)
            {
                // job is finished, copy stats out and free the job from the map
                memcpy(&ping.stats, &job->sequence.stats, sizeof(PingStats));
                jobs.erase(ping.hnd);
                ping.hnd = null_h32;
            }
            else if (ping.status == Sequence_Error)
            {
                // job errored, remove it and don't copy anything
                jobs.erase(ping.hnd);
                ping.hnd = null_h32;
            }
        }
        else {
            ping.status = Sequence_Error;
            ping.hnd = null_h32;
        }
    }

    return (ping.status > Sequence_Running);
}
