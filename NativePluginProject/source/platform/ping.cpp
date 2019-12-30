#include "ping.h"
#include "timer.h"
#include "platform.h"
#include <cmath>

static PingJobMap   jobs;
static PingJobQueue jobQueue;


#ifdef _WIN32
#include "ping_win32.cpp"
#else
#include "ping_linux.cpp"
#endif


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
    hdr.id      = (u16)platformGetPid();
    hdr.seq     = htons(seq);

    memset(
        buffer + sizeof(ICMPHeader),
        0xDA,
        packetSize - sizeof(ICMPHeader));

    hdr.checksum = checksum((u16*)buffer, packetSize);

    outHdr = hdr;
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
    else if (pingReply.id != (u16)platformGetPid()) {
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
            platform_closesocket(job.socket);
        }
    }

    job.sequence.status.store(status, std::memory_order_release);
    return status;
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

        size_t hostLen = strlen(host);
        sequence.host = (char*)malloc(hostLen+1);
        _strncpy_s(sequence.host, hostLen+1, host, hostLen);

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
                free(job->sequence.host);
                jobs.erase(ping.hnd);
                ping.hnd = null_h32;
            }
            else if (ping.status == Sequence_Error)
            {
                // job errored, remove it and don't copy anything
                free(job->sequence.host);
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
